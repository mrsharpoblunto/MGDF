#include "StdAfx.h"

#pragma warning(push)
#pragma warning(disable : 4091)
#include <Dbghelp.h>
#pragma warning(pop)

#include <mmsystem.h>

#include "MGDFApp.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"
#include "common/win32Exception.hpp"
#include "core.impl/MGDFHostBuilder.hpp"

void FatalErrorCallBack(const std::string &sender, const std::string &message);
void WriteMinidump();
DWORD WINAPI CrashDumpThread(LPVOID data);

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
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
  }
  virtual ~MemoryLeakChecker() { _CrtDumpMemoryLeaks(); }
};

static MemoryLeakChecker _memoryLeakChecker;
#endif

HANDLE _dumpEvent = nullptr;
HANDLE _dumpThread = nullptr;
bool _hasDumped = false;
std::unique_ptr<_MINIDUMP_EXCEPTION_INFORMATION> _dumpInfo;
MGDFApp *_application = nullptr;

D3DAPP_WNDPROC(MGDFAppWndProc, _application)

INT32 WINAPI WinMain(_In_ HINSTANCE const hInstance,
                     _In_opt_ HINSTANCE const hPreviousInstance,
                     _In_ LPSTR const lpCmdLine, _In_ INT32 const nCmdShow) {
  std::ignore = nCmdShow;
  std::ignore = lpCmdLine;
  std::ignore = hPreviousInstance;

  const HRESULT comHr = CoInitialize(NULL);

  timeBeginPeriod(1);  // set a higher resolution for timing calls

  Resources::Instance(hInstance);  // initialise the core resource locator

  _dumpEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  _dumpThread = CreateThread(nullptr, 0, CrashDumpThread, nullptr, 0, 0);
  SetUnhandledExceptionFilter(
      UnhandledExceptionCallBack);  // logs and stackdumps when any unexpected
                                    // errors occur

  SetErrorMode(SEM_NOGPFAULTERRORBOX);

  LOG("starting up...", MGDF_LOG_LOW);

  {
    // create the host object and related components
    ComObject<Host> host;
    if (FAILED(HostBuilder::TryCreateHost(host))) {
      LOG("failed to start up", MGDF_LOG_ERROR);
      return -1;
    }

    // create the application instance and initialise the window
    host->SetFatalErrorHandler(FatalErrorCallBack);
    _application = new MGDFApp(host, hInstance);
    _application->InitWindow("MGDF", MGDFAppWndProc);
    _application->Run();

    // dispose of the  and related components
    HostBuilder::DisposeHost(host);
  }

  LOG("shutting down...", MGDF_LOG_LOW);
  delete _application;

  timeEndPeriod(1);

  if (SUCCEEDED(comHr)) {
    CoUninitialize();
  }

  if (_dumpEvent) {
    CloseHandle(_dumpEvent);
  }
  if (_dumpThread) {
    CloseHandle(_dumpThread);
  }

  LOG("shut down successfully", MGDF_LOG_LOW);

  return 0;
}

/**
log & handle unexpected win32 errors before the exception is thrown
*/
LONG WINAPI
UnhandledExceptionCallBack(struct _EXCEPTION_POINTERS *pExceptionInfo) {
  if (pExceptionInfo) {
    LOG("WIN32 ERROR: " << Win32Exception::TranslateError(
            pExceptionInfo->ExceptionRecord->ExceptionCode),
        MGDF_LOG_ERROR);
    LOG("Generating Minidump file minidump.dmp...", MGDF_LOG_ERROR);

    _dumpInfo = std::make_unique<_MINIDUMP_EXCEPTION_INFORMATION>();
    _dumpInfo->ThreadId = GetCurrentThreadId();
    _dumpInfo->ExceptionPointers = pExceptionInfo;
    _dumpInfo->ClientPointers = 0;

    WriteMinidump();

    if (!ParameterManager::Instance().HasParameter("hideerrors")) {
      MessageBox(nullptr,
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
handle fatal errors explicitly invoked from the MGDF
*/
void FatalErrorCallBack(const std::string &sender, const std::string &message) {
  std::ignore = sender;
  WriteMinidump();

  if (!ParameterManager::Instance().HasParameter("hideerrors")) {
    MessageBox(nullptr,
               (message +
                "\r\n\r\nFor more information, please view the log file\r\n'" +
                Resources::ToString(Resources::Instance().LogFile()) + "'")
                   .c_str(),
               "FATAL ERROR", MB_OK);  // output a nasty error message
  }
}

void WriteMinidump() {
  if (!_hasDumped) {
    _hasDumped = true;
    // MiniDumpWriteDump() doesn't write callstack for the calling thread
    // correctly. use msdn-recommended work-around of spinning a thread to do
    // the writing
    SetEvent(_dumpEvent);
    WaitForSingleObject(_dumpThread, INFINITE);
  }
}

DWORD WINAPI CrashDumpThread(LPVOID data) {
  std::ignore = data;
  WaitForSingleObject(_dumpEvent, INFINITE);

  LOG("Generating Minidump file minidump.dmp...", MGDF_LOG_ERROR);
  HMODULE hDll = LoadLibraryW(L"DBGHELP.DLL");
  if (hDll) {
    MINIDUMPWRITEDUMP pDump =
        (MINIDUMPWRITEDUMP)GetProcAddress(hDll, "MiniDumpWriteDump");
    if (pDump) {
      HANDLE hFile = CreateFileW(
          (Resources::Instance().UserBaseDir() + L"minidump.dmp").c_str(),
          GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,
          FILE_ATTRIBUTE_NORMAL, NULL);

      if (hFile != INVALID_HANDLE_VALUE && _dumpInfo) {
        const BOOL ok =
            pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
                  MiniDumpNormal, _dumpInfo.get(), nullptr, nullptr);
        if (!ok) {
          LOG("Failed to save dump file", MGDF_LOG_ERROR);
        }
        CloseHandle(hFile);
      } else {
        LOG("Failed to save dump file", MGDF_LOG_ERROR);
      }
    }
  }
  Logger::Instance().Flush();
  _dumpInfo.reset();

  return 0;
}
