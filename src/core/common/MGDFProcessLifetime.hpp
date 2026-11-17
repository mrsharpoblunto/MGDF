#pragma once

#include <windows.h>

#include <atomic>
#include <memory>
#include <string>

namespace MGDF::core {

class UserInstanceGuard {
 public:
  ~UserInstanceGuard();
  static HRESULT Acquire(const std::wstring &folder,
                         std::shared_ptr<UserInstanceGuard> &guard);

 private:
  explicit UserInstanceGuard(HANDLE handle) : _handle(handle) {}
  HANDLE _handle;
};

class CrashExitState {
 public:
  static CrashExitState &Instance();
  void BeginCrash();
  bool TryNormalExit();
  [[noreturn]] void ExitNormally(UINT code);

 private:
  std::atomic<unsigned> _state{0};
};

}  // namespace MGDF::core
