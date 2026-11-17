#pragma once

#include <windows.h>

#include <string>
#include <string_view>

#include "CrashLayout.cs"

namespace MGDF::core {

struct CrashReporterOptions {
  std::wstring Executable;
  std::wstring GameName;
  std::wstring GameUid;
  std::wstring GameVersion;
  std::wstring FrameworkVersion;
  std::wstring IconPath;
  std::wstring UserFolder;
  std::wstring LogFile;
  std::wstring ServiceUrl;
  std::wstring PrivacyPolicyUrl;
  bool HideErrors = false;
};

class CrashReporter {
 public:
  static CrashReporter &Instance() {
    static CrashReporter reporter;
    return reporter;
  }

  ~CrashReporter();
  bool Start(const CrashReporterOptions &options);
  bool SignalCrash(PEXCEPTION_POINTERS exceptionPointers, DWORD timeout = 15000,
                   std::wstring_view errorText = {});

 private:
  CrashReporter() = default;
  CrashReporter(const CrashReporter &) = delete;
  CrashReporter &operator=(const CrashReporter &) = delete;
  void Close();

  HANDLE _crash = nullptr;
  HANDLE _dumpDone = nullptr;
  HANDLE _mapping = nullptr;
  HANDLE _reporter = nullptr;
  unsigned char *_data = nullptr;
  volatile LONG _signaled = 0;
};

}  // namespace MGDF::core
