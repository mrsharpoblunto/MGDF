#include "StdAfx.h"

#include <timeapi.h>

#include "MGDFApp.hpp"
#include "common/MGDFCrashReporter.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFParameterManager.hpp"
#include "common/MGDFResources.hpp"
#include "common/win32Exception.hpp"
#include "core.impl/MGDFHostBuilder.hpp"

void FatalErrorCallBack(const std::string &sender, const std::string &message);

LONG WINAPI UnhandledExceptionCallBack(PEXCEPTION_POINTERS pExceptionInfo);

extern "C" {
__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
__declspec(dllexport) extern const char *D3D12SDKPath = ".\\D3D12\\";
}

using namespace MGDF;
using namespace MGDF::core;

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

INT32 WINAPI WinMain(_In_ HINSTANCE const hInstance,
                     _In_opt_ HINSTANCE const hPreviousInstance,
                     _In_ LPSTR const lpCmdLine, _In_ INT32 const nCmdShow) {
#if defined(_DEBUG)
  ::_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  ::_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif

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
      return 1;
    }

    // create the application instance and initialise the window
    host->SetFatalErrorHandler(FatalErrorCallBack);
    MGDFApp app(host, hInstance);
    ComObject<IMGDFGame> game;
    host->GetGame(game.Assign());
    app.InitWindow(game->GetName());
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
  const auto error = pExceptionInfo
                         ? Win32Exception::TranslateError(
                               pExceptionInfo->ExceptionRecord->ExceptionCode)
                         : std::string{};
  if (pExceptionInfo) {
    LOG("WIN32 ERROR: " << error, MGDF_LOG_ERROR);
  }
  // The crash reporter reads the log file after this process exits.
  Logger::Instance().FlushSync();
  if (CrashReporter::Instance().SignalCrash(pExceptionInfo, 15000,
                                            Resources::ToWString(error))) {
    return EXCEPTION_EXECUTE_HANDLER;
  }
  if (pExceptionInfo &&
      !ParameterManager::Instance().HasParameter("hideerrors")) {
    ::MessageBox(nullptr,
                 ("Unexpected Win32 error\r\n\r\nFor more information, please "
                  "view the log file\r\n'" +
                  Resources::ToString(Resources::Instance().LogFile()) + "'")
                     .c_str(),
                 "FATAL ERROR", MB_OK);  // output a nasty error message
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

/**
handle fatal errors explicitly invoked from the MGDF core
*/
void FatalErrorCallBack(const std::string &sender, const std::string &message) {
  Logger::Instance().FlushSync();
  if (CrashReporter::Instance().SignalCrash(
          nullptr, 15000, Resources::ToWString(sender + ": " + message)))
    return;

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
