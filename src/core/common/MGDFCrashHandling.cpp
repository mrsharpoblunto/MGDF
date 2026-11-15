#include "stdafx.h"

#include "MGDFCrashHandling.hpp"

#include "MGDFCrashReporter.hpp"
#include "MGDFLoggerImpl.hpp"
#include "MGDFParameterManager.hpp"
#include "MGDFProcessLifetime.hpp"
#include "MGDFResources.hpp"
#include "win32Exception.hpp"

namespace MGDF::core {
LONG WINAPI HandleUnhandledException(PEXCEPTION_POINTERS pExceptionInfo) {
  CrashExitState::Instance().BeginCrash();
  const auto error = pExceptionInfo
                         ? Win32Exception::TranslateError(
                               pExceptionInfo->ExceptionRecord->ExceptionCode)
                         : std::string{};
  if (pExceptionInfo && !Logger::IsLockedByThisThread()) {
    LOG("WIN32 ERROR: " << error, MGDF_LOG_ERROR);
  }
  if (!Logger::IsLockedByThisThread()) Logger::Instance().FlushSync();
  if (CrashReporter::Instance().SignalCrash(pExceptionInfo, 15000,
                                            Resources::ToWString(error))) {
    TerminateProcess(
        GetCurrentProcess(),
        pExceptionInfo ? pExceptionInfo->ExceptionRecord->ExceptionCode : 1);
    return EXCEPTION_EXECUTE_HANDLER;
  }
  if (pExceptionInfo &&
      !ParameterManager::Instance().HasParameter("hideerrors")) {
    ::MessageBoxA(nullptr,
                  ("Unexpected Win32 error\r\n\r\nFor more information, please "
                   "view the log file\r\n'" +
                   Resources::ToString(Resources::Instance().LogFile()) + "'")
                      .c_str(),
                  "FATAL ERROR", MB_OK);
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

void HandleFatalError(const std::string &sender, const std::string &message) {
  CrashExitState::Instance().BeginCrash();
  if (!Logger::IsLockedByThisThread()) Logger::Instance().FlushSync();
  if (CrashReporter::Instance().SignalCrash(
          nullptr, 15000, Resources::ToWString(sender + ": " + message)))
    return;

  if (!ParameterManager::Instance().HasParameter("hideerrors")) {
    ::MessageBoxA(
        nullptr,
        (message +
         "\r\n\r\nFor more information, please view the log file\r\n'" +
         Resources::ToString(Resources::Instance().LogFile()) + "'")
            .c_str(),
        "FATAL ERROR", MB_OK);
  }
}

}  // namespace MGDF::core
