#include "stdafx.h"

#include "MGDFProcessLifetime.hpp"

#include <filesystem>

namespace MGDF::core {

UserInstanceGuard::~UserInstanceGuard() { CloseHandle(_handle); }

HRESULT UserInstanceGuard::Acquire(const std::wstring &folder,
                                   std::shared_ptr<UserInstanceGuard> &guard) {
  const auto path = std::filesystem::path(folder) / L".core.lock";
  const auto handle =
      CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                  OPEN_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
  if (handle == INVALID_HANDLE_VALUE) {
    const auto error = GetLastError();
    return error == ERROR_SHARING_VIOLATION || error == ERROR_LOCK_VIOLATION
               ? S_FALSE
               : HRESULT_FROM_WIN32(error);
  }
  guard.reset(new UserInstanceGuard(handle));
  return S_OK;
}

CrashExitState &CrashExitState::Instance() {
  static CrashExitState state;
  return state;
}

void CrashExitState::BeginCrash() {
  unsigned running = 0;
  _state.compare_exchange_strong(running, 1);
}

bool CrashExitState::TryNormalExit() {
  unsigned running = 0;
  return _state.compare_exchange_strong(running, 2);
}

[[noreturn]] void CrashExitState::ExitNormally(UINT code) {
  if (TryNormalExit()) ExitProcess(code);
  WaitForSingleObject(GetCurrentProcess(), INFINITE);
  std::terminate();
}

}  // namespace MGDF::core
