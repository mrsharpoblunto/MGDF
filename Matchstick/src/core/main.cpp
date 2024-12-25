#include "StdAfx.h"

#pragma warning(push)
#pragma warning(disable : 4091)
#include <Dbghelp.h>
#pragma warning(pop)

#include <timeapi.h>

#include "MGDFApp.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"
#include "common/win32Exception.hpp"
#include "core.impl/MGDFHostBuilder.hpp"

void FatalErrorCallBack(const std::string &sender, const std::string &message);
void WriteMinidump(PEXCEPTION_POINTERS exceptionInfo);

LONG WINAPI
UnhandledExceptionCallBack(struct _EXCEPTION_POINTERS *pExceptionInfo);
typedef BOOL(WINAPI *MINIDUMPWRITEDUMP)(
    HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
    CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
    CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
    CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

using namespace MGDF;
using namespace MGDF::core;

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)

// track memory leaks
class MemoryLeakChecker {
 public:
  MemoryLeakChecker() {
    ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    ::_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
  }
  virtual ~MemoryLeakChecker() { ::_CrtDumpMemoryLeaks(); }
};

static MemoryLeakChecker _memoryLeakChecker;
#endif

struct DumpData {
  DWORD processId;
  DWORD threadId;
  LPEXCEPTION_POINTERS exceptionPointers;
};

HWND _launcherWindow = nullptr;
bool _hasDumped = false;

INT32 WINAPI WinMain(_In_ HINSTANCE const hInstance,
                     _In_opt_ HINSTANCE const hPreviousInstance,
                     _In_ LPSTR const lpCmdLine, _In_ INT32 const nCmdShow) {
  std::ignore = nCmdShow;
  std::ignore = lpCmdLine;
  std::ignore = hPreviousInstance;

  ::SetUnhandledExceptionFilter(
      UnhandledExceptionCallBack);  // logs and stackdumps when any unexpected
                                    // errors occur

  const HRESULT comHr = ::CoInitialize(NULL);

  ::timeBeginPeriod(1);  // set a higher resolution for timing calls

  Resources::Instance(hInstance);  // initialise the core resource locator

  ::SetErrorMode(SEM_NOGPFAULTERRORBOX);
  LOG("starting up...", MGDF_LOG_LOW);

  {
    // create the host object and related components
    ComObject<Host> host;
    if (FAILED(HostBuilder::TryCreateHost(host))) {
      LOG("failed to start up", MGDF_LOG_ERROR);
      return -1;
    }

    // record the launcher window handle so we can request a minidump from the
    // launcher process if we encounter an error later
    if (ParameterManager::Instance().HasParameter("launcherhandle")) {
      const std::string launcherHandle =
          ParameterManager::Instance().GetParameter("launcherhandle");
      _launcherWindow = (HWND)std::stoull(launcherHandle);
    }

    // create the application instance and initialise the window
    host->SetFatalErrorHandler(FatalErrorCallBack);
    MGDFApp app(host, hInstance);
    app.InitWindow("MGDF");
    app.Run();

    LOG("shutting down...", MGDF_LOG_LOW);
  }

  ::timeEndPeriod(1);

  if (SUCCEEDED(comHr)) {
    ::CoUninitialize();
  }

  LOG("shut down successfully", MGDF_LOG_LOW);

  return 0;
}

/**
log & handle unexpected win32 errors before the exception is thrown
*/
LONG WINAPI UnhandledExceptionCallBack(PEXCEPTION_POINTERS pExceptionInfo) {
  if (pExceptionInfo) {
    LOG("WIN32 ERROR: " << Win32Exception::TranslateError(
            pExceptionInfo->ExceptionRecord->ExceptionCode),
        MGDF_LOG_ERROR);

    WriteMinidump(pExceptionInfo);

    if (!ParameterManager::Instance().HasParameter("hideerrors")) {
      ::MessageBox(
          nullptr,
          ("Unexpected Win32 error\r\n\r\nFor more information, please "
           "view the log file\r\n'" +
           Resources::ToString(Resources::Instance().LogFile()) + "'")
              .c_str(),
          "FATAL ERROR", MB_OK);  // output a nasty error message
    }
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

/**
handle fatal errors explicitly invoked from the MGDF core
*/
void FatalErrorCallBack(const std::string &sender, const std::string &message) {
  std::ignore = sender;
  WriteMinidump(nullptr);

  if (!ParameterManager::Instance().HasParameter("hideerrors")) {
    ::MessageBox(
        nullptr,
        (message +
         "\r\n\r\nFor more information, please view the log file\r\n'" +
         Resources::ToString(Resources::Instance().LogFile()) + "'")
            .c_str(),
        "FATAL ERROR", MB_OK);  // output a nasty error message
  }
}

void WriteMinidump(PEXCEPTION_POINTERS exceptionInfo) {
  if (!_hasDumped) {
    _hasDumped = true;
    Logger::Instance().FlushSync();
    if (_launcherWindow) {
      DumpData data = {::GetCurrentProcessId(), ::GetCurrentThreadId(),
                       exceptionInfo};
      COPYDATASTRUCT cds;
      cds.dwData = 1;
      cds.cbData = sizeof(DumpData);
      cds.lpData = &data;
      ::SendMessageA(_launcherWindow, WM_COPYDATA, (WPARAM) nullptr,
                     (LPARAM)&cds);
    }
  }
}
