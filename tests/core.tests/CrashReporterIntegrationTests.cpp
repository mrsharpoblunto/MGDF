#include "stdafx.h"

#include <DbgHelp.h>

#include <filesystem>
#include <fstream>

#include "../../src/core/common/MGDFCrashReporter.hpp"
#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/core.impl/MGDFSaveManagerImpl.hpp"

using namespace MGDF::core;

namespace {
class CrashReporterIntegrationTests : public ::testing::Test {
 protected:
  void SetUp() override {
    Resources::Instance(GetModuleHandleW(nullptr));
    std::string id;
    ASSERT_TRUE(GuidString(id));
    _root = std::filesystem::path(Resources::Instance().RootDir()) /
            ("crash-test-" + id);
    std::filesystem::create_directories(_root);
  }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(_root, error);
  }
  void RunChild(const wchar_t *mode, DWORD expectedExit) {
    const auto executable = Resources::Instance().RootDir() + L"core.tests.exe";
    auto command = L"\"" + executable + L"\" --crash-reporter-child \"" +
                   _root.wstring() + L"\" " + mode;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    ASSERT_TRUE(CreateProcessW(executable.c_str(), command.data(), nullptr,
                               nullptr, FALSE, CREATE_NO_WINDOW, nullptr,
                               nullptr, &startup, &process));
    CloseHandle(process.hThread);
    const auto wait = WaitForSingleObject(process.hProcess, 30000);
    if (wait != WAIT_OBJECT_0) {
      TerminateProcess(process.hProcess, 4);
      WaitForSingleObject(process.hProcess, 5000);
    }
    DWORD exit = 0;
    GetExitCodeProcess(process.hProcess, &exit);
    CloseHandle(process.hProcess);
    ASSERT_EQ(WAIT_OBJECT_0, wait);
    ASSERT_EQ(expectedExit, exit);
  }
  std::filesystem::path DumpFile() const {
    for (const auto &entry : std::filesystem::directory_iterator(_root))
      if (entry.path().extension() == L".dmp") return entry.path();
    return {};
  }
  std::filesystem::path _root;
};
}  // namespace

TEST_F(CrashReporterIntegrationTests, HiddenCrashWritesExceptionDump) {
  ASSERT_NO_FATAL_FAILURE(RunChild(L"crash", EXCEPTION_ACCESS_VIOLATION));
  std::ifstream dump(DumpFile(), std::ios::binary);
  ASSERT_TRUE(dump.good());
  MINIDUMP_HEADER header{};
  ASSERT_TRUE(dump.read(reinterpret_cast<char *>(&header), sizeof(header)));
  ASSERT_EQ(MINIDUMP_SIGNATURE, header.Signature);
  dump.seekg(header.StreamDirectoryRva);
  std::vector<MINIDUMP_DIRECTORY> directory(header.NumberOfStreams);
  ASSERT_TRUE(dump.read(reinterpret_cast<char *>(directory.data()),
                        directory.size() * sizeof(MINIDUMP_DIRECTORY)));
  const auto exception = std::find_if(
      directory.begin(), directory.end(),
      [](const auto &entry) { return entry.StreamType == ExceptionStream; });
  ASSERT_NE(directory.end(), exception);
  dump.seekg(exception->Location.Rva);
  MINIDUMP_EXCEPTION_STREAM record{};
  ASSERT_TRUE(dump.read(reinterpret_cast<char *>(&record), sizeof(record)));
  EXPECT_NE(0U, record.ThreadId);
  EXPECT_EQ(EXCEPTION_ACCESS_VIOLATION, record.ExceptionRecord.ExceptionCode);
  EXPECT_NE(0ULL, record.ExceptionRecord.ExceptionAddress);
}

TEST_F(CrashReporterIntegrationTests, HiddenFatalErrorWritesDump) {
  ASSERT_NO_FATAL_FAILURE(RunChild(L"fatal", 1));
  std::ifstream dump(DumpFile(), std::ios::binary);
  MINIDUMP_HEADER header{};
  ASSERT_TRUE(dump.read(reinterpret_cast<char *>(&header), sizeof(header)));
  EXPECT_EQ(MINIDUMP_SIGNATURE, header.Signature);
}

int RunCrashReporterChild(const char *folder, const char *mode) {
  Resources::Instance(GetModuleHandleW(nullptr));
  SetErrorMode(SEM_NOGPFAULTERRORBOX);
  const std::filesystem::path root(folder);
  std::ofstream(root / "corelog.txt") << "Crash test log";
  CrashReporterOptions options{
      .Executable = Resources::Instance().RootDir() + L"crashreporter.exe",
      .GameName = L"Crash test \"game\"",
      .GameUid = L"crash-test",
      .GameVersion = L"1.0",
      .FrameworkVersion = L"1.0",
      .UserFolder = root.wstring() + L"\\",
      .LogFile = (root / "corelog.txt").wstring(),
      .ServiceUrl = L"http://127.0.0.1:1/unused",
      .HideErrors = true};
  const std::string selected(mode);
  if (selected == "reporter-exit" || selected == "reporter-timeout") {
    options.Executable = Resources::Instance().RootDir() + L"core.tests.exe";
    SetEnvironmentVariableA("MGDF_TEST_REPORTER_MODE", mode);
  }
  if (!CrashReporter::Instance().Start(options)) return 2;
  if (selected == "reporter-exit" || selected == "reporter-timeout")
    return CrashReporter::Instance().SignalCrash(nullptr, selected == "reporter-timeout" ? 0 : 15000) ? 3 : 0;
  SetUnhandledExceptionFilter([](PEXCEPTION_POINTERS info) -> LONG {
    CrashReporter::Instance().SignalCrash(info);
    return EXCEPTION_EXECUTE_HANDLER;
  });
  if (std::string(mode) == "crash") {
    RaiseException(EXCEPTION_ACCESS_VIOLATION, EXCEPTION_NONCONTINUABLE, 0,
                   nullptr);
    return 3;
  }
  return CrashReporter::Instance().SignalCrash(nullptr) ? 1 : 3;
}

TEST_F(CrashReporterIntegrationTests, ReporterExitFallsBack) {
  ASSERT_NO_FATAL_FAILURE(RunChild(L"reporter-exit", 0));
}

TEST_F(CrashReporterIntegrationTests, ReporterTimeoutFallsBack) {
  ASSERT_NO_FATAL_FAILURE(RunChild(L"reporter-timeout", 0));
}

TEST_F(CrashReporterIntegrationTests, RunsKeepSeparateDumps) {
  ASSERT_NO_FATAL_FAILURE(RunChild(L"fatal", 1));
  ASSERT_NO_FATAL_FAILURE(RunChild(L"fatal", 1));
  size_t dumps = 0;
  for (const auto &entry : std::filesystem::directory_iterator(_root))
    if (entry.path().extension() == L".dmp") ++dumps;
  EXPECT_EQ(2U, dumps);
}

int RunFakeReporter(int argc, char **argv) {
  std::map<std::string, HANDLE> handles;
  for (int i = 1; i + 1 < argc; i += 2)
    handles[argv[i]] = reinterpret_cast<HANDLE>(std::strtoull(argv[i + 1], nullptr, 10));
  if (handles.contains("--ready-event")) SetEvent(handles["--ready-event"]);
  char mode[64]{};
  GetEnvironmentVariableA("MGDF_TEST_REPORTER_MODE", mode, sizeof(mode));
  if (std::string(mode) == "reporter-timeout")
    WaitForSingleObject(handles["--process-handle"], 30000);
  return 0;
}
