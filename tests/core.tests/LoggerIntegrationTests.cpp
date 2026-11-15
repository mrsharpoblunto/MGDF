#include "stdafx.h"

#include <filesystem>
#include <fstream>
#include <future>

#include "../../src/core/common/MGDFLoggerImpl.hpp"
#include "../../src/core/common/MGDFProcessLifetime.hpp"

using namespace MGDF::core;
using namespace std::chrono_literals;

namespace {
class Event {
 public:
  explicit Event(const wchar_t *name = nullptr)
      : Handle(CreateEventW(nullptr, TRUE, FALSE, name)) {}
  ~Event() { CloseHandle(Handle); }
  void Set() const { SetEvent(Handle); }
  bool Wait(DWORD timeout = 5000) const {
    return WaitForSingleObject(Handle, timeout) == WAIT_OBJECT_0;
  }
  HANDLE Handle;
};

std::string Read(const std::filesystem::path &file) {
  std::ifstream input(file);
  return {std::istreambuf_iterator<char>(input), {}};
}

class LoggerIntegrationTests : public ::testing::Test {
 protected:
  void SetUp() override {
    GUID guid{};
    ASSERT_EQ(S_OK, CoCreateGuid(&guid));
    wchar_t text[40]{};
    ASSERT_NE(0, StringFromGUID2(guid, text, 40));
    _id = text;
    _root = std::filesystem::temp_directory_path() / (L"mgdf-log-" + _id);
    std::filesystem::create_directories(_root);
  }
  void TearDown() override {
    std::error_code error;
    std::filesystem::remove_all(_root, error);
  }
  DWORD Child(const std::wstring &arguments) {
    wchar_t executable[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executable, MAX_PATH);
    auto command = L"\"" + std::wstring(executable) + L"\" " + arguments;
    STARTUPINFOW startup{sizeof(startup)};
    PROCESS_INFORMATION child{};
    if (!CreateProcessW(executable, command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child))
      return 99;
    CloseHandle(child.hThread);
    const auto wait = WaitForSingleObject(child.hProcess, 10000);
    if (wait != WAIT_OBJECT_0) {
      TerminateProcess(child.hProcess, 98);
      WaitForSingleObject(child.hProcess, 5000);
    }
    DWORD code = 99;
    GetExitCodeProcess(child.hProcess, &code);
    CloseHandle(child.hProcess);
    return code;
  }
  std::filesystem::path _root;
  std::wstring _id;
};
}  // namespace

TEST_F(LoggerIntegrationTests, SynchronousFlushWaitsForTheTakenBatch) {
  Event writing, release, flushing;
  std::atomic<bool> first{true};
  Logger logger((_root / "log.txt").wstring(), [&](Logger::Point point) {
    if (point == Logger::Point::Write && first.exchange(false)) {
      writing.Set();
      EXPECT_TRUE(release.Wait());
    }
  });
  logger.Log("test", "first", MGDF_LOG_ERROR);
  logger.Flush();
  const bool ready = writing.Wait();
  if (!ready) {
    release.Set();
    FAIL() << "No write began";
  }
  logger.Log("test", "second", MGDF_LOG_ERROR);
  auto flush = std::async(std::launch::async, [&] {
    flushing.Set();
    logger.FlushSync();
  });
  EXPECT_TRUE(flushing.Wait());
  const auto blocked = flush.wait_for(100ms);
  release.Set();
  ASSERT_EQ(std::future_status::ready, flush.wait_for(5s));
  flush.get();
  logger.Stop();
  EXPECT_EQ(std::future_status::timeout, blocked);
  const auto text = Read(_root / "log.txt");
  EXPECT_LT(text.find("first"), text.find("second"));
}

TEST_F(LoggerIntegrationTests, StopWritesLinesAddedDuringAWrite) {
  Event writing, release, stopping;
  std::atomic<bool> first{true};
  Logger logger((_root / "log.txt").wstring(), [&](Logger::Point point) {
    if (point == Logger::Point::Write && first.exchange(false)) {
      writing.Set();
      EXPECT_TRUE(release.Wait());
    }
    if (point == Logger::Point::Stop) stopping.Set();
  });
  logger.Log("test", "first", MGDF_LOG_ERROR);
  logger.Flush();
  const bool ready = writing.Wait();
  if (!ready) {
    release.Set();
    FAIL() << "No write began";
  }
  auto stop = std::async(std::launch::async, [&] { logger.Stop(); });
  EXPECT_TRUE(stopping.Wait());
  logger.Log("test", "last", MGDF_LOG_ERROR);
  release.Set();
  ASSERT_EQ(std::future_status::ready, stop.wait_for(5s));
  stop.get();
  EXPECT_NE(std::string::npos, Read(_root / "log.txt").find("last"));
}

TEST_F(LoggerIntegrationTests, RemoteSendDoesNotHoldTheWriteLock) {
  Event sending, release;
  Logger logger((_root / "log.txt").wstring());
  logger.SetRemoteEndpoint("local-test", [&](const auto &, const auto &) {
    sending.Set();
    EXPECT_TRUE(release.Wait());
  });
  logger.Log("test", "first", MGDF_LOG_ERROR);
  logger.Flush();
  const bool ready = sending.Wait();
  if (!ready) {
    release.Set();
    FAIL() << "No remote send began";
  }
  logger.Log("test", "second", MGDF_LOG_ERROR);
  auto flush = std::async(std::launch::async, [&] { logger.FlushSync(); });
  const auto finished = flush.wait_for(5s);
  release.Set();
  ASSERT_EQ(std::future_status::ready, finished);
  flush.get();
  logger.Stop();
  EXPECT_NE(std::string::npos, Read(_root / "log.txt").find("second"));
}

TEST_F(LoggerIntegrationTests, ReentrantCrashSkipsTheLoggerLock) {
  EXPECT_EQ(0U, Child(L"--logger-reentry \"" + (_root / "log.txt").wstring() +
                      L"\""));
}

TEST_F(LoggerIntegrationTests, ANewLogKeepsThePreviousFileObject) {
  const auto path = _root / "log.txt";
  {
    Logger first(path.wstring());
    first.Log("test", "old", MGDF_LOG_ERROR);
    first.Stop();
  }
  const auto handle =
      CreateFileW(path.c_str(), GENERIC_READ,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  ASSERT_NE(INVALID_HANDLE_VALUE, handle);
  {
    Logger second(path.wstring());
    second.Log("test", "new", MGDF_LOG_ERROR);
    second.Stop();
  }
  char bytes[1024]{};
  DWORD count = 0;
  const bool read = ReadFile(handle, bytes, sizeof(bytes), &count, nullptr);
  CloseHandle(handle);
  ASSERT_TRUE(read);
  EXPECT_NE(std::string::npos, std::string(bytes, count).find("old"));
  EXPECT_EQ(std::string::npos, std::string(bytes, count).find("new"));
  EXPECT_NE(std::string::npos, Read(path).find("new"));
}

TEST_F(LoggerIntegrationTests, UserFolderGuardBlocksOnlyTheSameFolder) {
  std::shared_ptr<UserInstanceGuard> guard;
  ASSERT_EQ(S_OK, UserInstanceGuard::Acquire(_root.wstring(), guard));
  std::ofstream(_root / "coreLog.txt") << "keep this log";
  EXPECT_EQ(0U, Child(L"--instance-probe \"" + _root.wstring() + L"\""));
  EXPECT_EQ("keep this log", Read(_root / "coreLog.txt"));
  const auto other = _root / "other";
  std::filesystem::create_directories(other);
  EXPECT_EQ(17U, Child(L"--instance-probe \"" + other.wstring() + L"\""));
  guard.reset();
  EXPECT_EQ(17U, Child(L"--instance-probe \"" + _root.wstring() + L"\""));
}

TEST_F(LoggerIntegrationTests, NormalExitYieldsToAnActiveCrash) {
  const auto readyName = L"Local\\mgdf-crash-ready-" + _id;
  const auto releaseName = L"Local\\mgdf-crash-exit-" + _id;
  Event ready(readyName.c_str()), release(releaseName.c_str());
  auto child = std::async(std::launch::async, [&] {
    return Child(L"--exit-crash " + readyName + L" " + releaseName);
  });
  const bool entered = ready.Wait();
  release.Set();
  ASSERT_TRUE(entered);
  ASSERT_EQ(std::future_status::ready, child.wait_for(5s));
  EXPECT_EQ(17U, child.get());
}

int RunLoggerChild(int argc, char **argv) {
  const std::string mode(argv[1]);
  if (mode == "--logger-reentry") {
    Logger *target = nullptr;
    bool skipped = false;
    Logger logger(std::filesystem::path(argv[2]).wstring(),
                  [&](Logger::Point point) {
                    if (point == Logger::Point::Append)
                      skipped = !target->LogCrash("crash", "skip");
                  });
    target = &logger;
    logger.Log("test", "outer", MGDF_LOG_ERROR);
    logger.Stop();
    return skipped ? 0 : 3;
  }
  if (mode == "--instance-probe") {
    std::shared_ptr<UserInstanceGuard> guard;
    const auto result = UserInstanceGuard::Acquire(
        std::filesystem::path(argv[2]).wstring(), guard);
    if (result == S_FALSE) return 0;
    if (result != S_OK) return 3;
    std::ofstream(std::filesystem::path(argv[2]) / "coreLog.txt")
        << "child owns the folder";
    return 17;
  }
  if (argc == 4 && mode == "--exit-crash") {
    auto &state = CrashExitState::Instance();
    state.BeginCrash();
    if (state.TryNormalExit()) return 3;
    const auto ready = OpenEventA(EVENT_MODIFY_STATE, FALSE, argv[2]);
    const auto release = OpenEventA(SYNCHRONIZE, FALSE, argv[3]);
    if (!ready || !release) return 4;
    std::thread crash([=] {
      if (WaitForSingleObject(release, 5000) == WAIT_OBJECT_0)
        TerminateProcess(GetCurrentProcess(), 17);
      TerminateProcess(GetCurrentProcess(), 4);
    });
    crash.detach();
    SetEvent(ready);
    state.ExitNormally(0);
  }
  return 3;
}
