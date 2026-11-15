#include "StdAfx.h"

#include <timeapi.h>

#include "MGDFApp.hpp"
#include "common/MGDFCrashHandling.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFParameterManager.hpp"
#include "common/MGDFProcessLifetime.hpp"
#include "common/MGDFResources.hpp"
#include "common/win32Exception.hpp"
#include "core.impl/MGDFHostBuilder.hpp"

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
      HandleUnhandledException);  // logs and stackdumps when any unexpected
                                  // errors occur

  const HRESULT comHr = ::CoInitialize(NULL);

  ::timeBeginPeriod(1);  // set a higher resolution for timing calls

  Resources::Instance(hInstance);  // initialise the core resource locator

  ::SetErrorMode(SEM_NOGPFAULTERRORBOX);
  LOG("starting up...", MGDF_LOG_LOW);

  std::shared_ptr<UserInstanceGuard> instanceGuard;
  {
    // create the host object and related components
    ComObject<Host> host;
    const auto result = HostBuilder::TryCreateHost(host, &instanceGuard);
    if (result == S_FALSE) return 0;
    if (FAILED(result)) {
      LOG("failed to start up", MGDF_LOG_ERROR);
      Logger::Instance().Stop();
      CrashExitState::Instance().ExitNormally(1);
    }

    // create the application instance and initialise the window
    host->SetFatalErrorHandler(HandleFatalError);
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

  Logger::Instance().Stop();
  CrashExitState::Instance().ExitNormally(0);
}
