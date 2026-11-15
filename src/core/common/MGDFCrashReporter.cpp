#include "stdafx.h"

#include "MGDFCrashReporter.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace MGDF::core {
namespace {

template <typename T>
void SetField(unsigned char *data, CrashLayout field, T value) {
  std::memcpy(data + static_cast<size_t>(field), &value, sizeof(value));
}

std::wstring Quote(const std::wstring &value) {
  std::wstring result = L"\"";
  size_t slashes = 0;
  for (const auto ch : value) {
    if (ch == L'\\') {
      ++slashes;
    } else {
      result.append(slashes * (ch == L'"' ? 2 : 1), L'\\');
      slashes = 0;
      if (ch == L'"') result += L'\\';
      result += ch;
    }
  }
  result.append(slashes * 2, L'\\');
  return result + L'"';
}

std::wstring HandleArgument(HANDLE handle) {
  return std::to_wstring(reinterpret_cast<UINT_PTR>(handle));
}

}  // namespace

CrashReporter::~CrashReporter() { Close(); }

void CrashReporter::Close() {
  if (_data) UnmapViewOfFile(_data);
  _data = nullptr;
  for (auto handle : {_crash, _dumpDone, _mapping, _reporter}) {
    if (handle) CloseHandle(handle);
  }
  _crash = _dumpDone = _mapping = _reporter = nullptr;
}

bool CrashReporter::Start(const CrashReporterOptions &options) {
  if (_reporter) return true;
  if (options.ServiceUrl.empty() ||
      GetFileAttributesW(options.Executable.c_str()) ==
          INVALID_FILE_ATTRIBUTES) {
    return false;
  }

  SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
  _crash = CreateEventW(&security, TRUE, FALSE, nullptr);
  _dumpDone = CreateEventW(&security, TRUE, FALSE, nullptr);
  _mapping =
      CreateFileMappingW(INVALID_HANDLE_VALUE, &security, PAGE_READWRITE, 0,
                         static_cast<DWORD>(CrashLayout::Size), nullptr);
  const auto ready = CreateEventW(&security, TRUE, FALSE, nullptr);
  const auto log =
      CreateFileW(options.LogFile.c_str(), GENERIC_READ,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  HANDLE process = nullptr;
  DuplicateHandle(
      GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &process,
      PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | SYNCHRONIZE, TRUE, 0);
  if (_mapping) {
    _data = static_cast<unsigned char *>(MapViewOfFile(
        _mapping, FILE_MAP_WRITE, 0, 0, static_cast<DWORD>(CrashLayout::Size)));
  }
  bool started = false;
  if (_crash && _dumpDone && _data && ready && process &&
      log != INVALID_HANDLE_VALUE) {
    ZeroMemory(_data, static_cast<size_t>(CrashLayout::Size));
    std::wstring command = Quote(options.Executable);
    const auto add = [&](const wchar_t *name, const std::wstring &value) {
      command += L" --" + std::wstring(name) + L" " + Quote(value);
    };
    add(L"process-id", std::to_wstring(GetCurrentProcessId()));
    add(L"process-handle", HandleArgument(process));
    add(L"crash-event", HandleArgument(_crash));
    add(L"dump-event", HandleArgument(_dumpDone));
    add(L"ready-event", HandleArgument(ready));
    add(L"mapping-handle", HandleArgument(_mapping));
    add(L"game-name", options.GameName);
    add(L"game-uid", options.GameUid);
    add(L"game-version", options.GameVersion);
    add(L"framework-version", options.FrameworkVersion);
    add(L"icon", options.IconPath);
    add(L"user-folder", options.UserFolder);
    add(L"log-handle", HandleArgument(log));
    add(L"service", options.ServiceUrl);
    add(L"privacy-policy", options.PrivacyPolicyUrl);
    add(L"hide-errors", options.HideErrors ? L"true" : L"false");

    SIZE_T size = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &size);
    std::vector<unsigned char> storage(size);
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList =
        reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0,
                                          &size)) {
      // Inherit only the reporter's handles, even if other handles are
      // inheritable.
      std::array handles{process, _crash, _dumpDone, ready, _mapping, log};
      if (UpdateProcThreadAttribute(
              startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
              handles.data(), sizeof(handles), nullptr, nullptr)) {
        PROCESS_INFORMATION child{};
        if (CreateProcessW(options.Executable.c_str(), command.data(), nullptr,
                           nullptr, TRUE,
                           EXTENDED_STARTUPINFO_PRESENT | CREATE_NO_WINDOW,
                           nullptr, nullptr, &startup.StartupInfo, &child)) {
          CloseHandle(child.hThread);
          _reporter = child.hProcess;
          const HANDLE wait[]{ready, _reporter};
          started =
              WaitForMultipleObjects(2, wait, FALSE, 5000) == WAIT_OBJECT_0;
          if (!started) TerminateProcess(_reporter, 1);
        }
      }
      DeleteProcThreadAttributeList(startup.lpAttributeList);
    }
  }
  if (log != INVALID_HANDLE_VALUE) CloseHandle(log);
  if (ready) CloseHandle(ready);
  if (process) CloseHandle(process);
  // Later child processes must not retain this channel.
  for (auto handle : {_crash, _dumpDone, _mapping}) {
    if (handle) SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0);
  }
  if (!started) Close();
  return started;
}

bool CrashReporter::SignalCrash(PEXCEPTION_POINTERS exceptionPointers,
                                DWORD timeout, std::wstring_view errorText) {
  if (!_reporter || WaitForSingleObject(_reporter, 0) != WAIT_TIMEOUT) {
    return false;
  }
  if (InterlockedCompareExchange(&_signaled, 1, 0) == 0) {
    SetField(_data, CrashLayout::ThreadId, GetCurrentThreadId());
    SetField(_data, CrashLayout::ExceptionPointers,
             reinterpret_cast<UINT64>(exceptionPointers));
    auto length = (std::min)(errorText.size(),
                           static_cast<size_t>(CrashLayout::ErrorLimit));
    if (length && errorText[length - 1] >= 0xD800 &&
        errorText[length - 1] <= 0xDBFF)
      --length;
    SetField(_data, CrashLayout::ErrorLength, static_cast<DWORD>(length));
    if (length)
      std::memcpy(_data + static_cast<size_t>(CrashLayout::ErrorText),
                  errorText.data(), length * sizeof(wchar_t));
    SetEvent(_crash);
  }
  const HANDLE wait[]{_dumpDone, _reporter};
  WaitForMultipleObjects(2, wait, FALSE, timeout);
  return WaitForSingleObject(_dumpDone, 0) == WAIT_OBJECT_0;
}

}  // namespace MGDF::core
