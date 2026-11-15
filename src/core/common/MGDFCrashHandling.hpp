#pragma once

#include <windows.h>

#include <string>

namespace MGDF::core {
LONG WINAPI HandleUnhandledException(PEXCEPTION_POINTERS info);
void HandleFatalError(const std::string &sender, const std::string &message);
}  // namespace MGDF::core
