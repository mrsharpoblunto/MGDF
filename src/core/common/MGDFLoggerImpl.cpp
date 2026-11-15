#include "stdafx.h"

#include "MGDFLoggerImpl.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <nlohmann/json.hpp>

#include "MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::chrono_literals;

namespace MGDF {
namespace core {

// wait for either this many log entries, or the timeout to
// periodically flush logs
#define LOG_BUFFER_SIZE 100
#define LOG_FLUSH_TIMEOUT 5s

size_t GetTimeStamp() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

// milliseconds since the host started, so the ordered log doubles as a
// coarse profile without each caller having to time itself
double GetElapsed() {
  static const auto start = std::chrono::steady_clock::now();
  return std::chrono::duration<double, std::milli>(
             std::chrono::steady_clock::now() - start)
      .count();
}

void MGDFLog(std::function<void(std::ostringstream &)> msg, MGDFLogLevel level,
             const char *file, int line) {
  if (level <= Logger::Instance().GetLoggingLevel()) {
    std::ostringstream ss;
    ss << file << "(" << line << ")";
    std::ostringstream ms;
    msg(ms);
    Logger::Instance().Log(ss.str().c_str(), ms.str().c_str(), level);
  }
}

namespace {
thread_local unsigned loggerDepth = 0;
struct LoggerSection {
  LoggerSection() { ++loggerDepth; }
  ~LoggerSection() { --loggerDepth; }
};
}  // namespace

void Logger::Log(const char *sender, const char *message, MGDFLogLevel level) {
  _ASSERTE(sender);
  _ASSERTE(message);

  if (level <= _level.load()) {
    std::ostringstream prefixed;
    prefixed << "[t=" << std::fixed << std::setprecision(1) << GetElapsed()
             << "ms] " << message;
    const auto text = prefixed.str();
#if defined(_DEBUG)
    std::ostringstream stream;
    stream << sender << ": " << text << "\n";
    OutputDebugString(stream.str().c_str());
#endif
    {
      LoggerSection section;
      std::unique_lock<std::mutex> lock(_mutex);
      if (_closed) return;
      if (_hook) _hook(Point::Append);
      const auto timestamp = GetTimeStamp();
      _events.push_back(LogEntry{.Level = level,
                                 .Timestamp = timestamp,
                                 .Sender = sender,
                                 .Message = text});
      if (_events.size() >= LOG_BUFFER_SIZE) {
        lock.unlock();
        _cv.notify_one();
      }
    }
  }
}

void Logger::Flush() { _cv.notify_one(); }

bool Logger::IsLockedByThisThread() { return loggerDepth != 0; }

bool Logger::LogCrash(const char *sender, const char *message) {
  if (IsLockedByThisThread()) return false;
  Log(sender, message, MGDF_LOG_ERROR);
  FlushSync();
  return true;
}

void Logger::Write(const std::vector<LogEntry> &events) {
  if (events.empty()) return;
  if (_hook) _hook(Point::Write);
  std::ofstream output(_filename.c_str(), std::ios::app);
  for (const auto &event : events)
    output << event.Sender << ": " << event.Message << std::endl;
}

void Logger::FlushSync() {
  LoggerSection section;
  std::lock_guard<std::mutex> write(_writeMutex);
  std::vector<LogEntry> events;
  {
    std::lock_guard<std::mutex> lock(_mutex);
    events.swap(_events);
  }
  Write(events);
}

Logger::Logger() : Logger(Resources::Instance().LogFile()) {}

Logger::Logger(const std::wstring &filename, std::function<void(Point)> hook)
    : _hook(std::move(hook)), _runLogger(true) {
  SetLoggingLevel(MGDF_LOG_MEDIUM);
  SetOutputFile(filename);
  _flushThread = std::thread([this]() {
    for (;;) {
      {
        LoggerSection section;
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait_for(lock, LOG_FLUSH_TIMEOUT,
                     [this] { return !_runLogger || !_events.empty(); });
      }
      std::vector<LogEntry> events;
      std::function<void(const std::string &, const std::string &)>
          remoteSender;
      bool stop;
      {
        LoggerSection section;
        std::lock_guard<std::mutex> write(_writeMutex);
        {
          std::lock_guard<std::mutex> lock(_mutex);
          events.swap(_events);
          stop = !_runLogger;
          if (!_remoteEndpoint.empty()) remoteSender = _remoteSender;
        }
        Write(events);
      }
      if (!events.empty()) {
        if (remoteSender) {
          nlohmann::json root;
          auto &streams = root["streams"] = nlohmann::json::array();
          auto &stream = streams.emplace_back(nlohmann::json::object());
          auto &streamLabel = stream["stream"] = nlohmann::json::object();
          streamLabel["label"] = "MGDF";
          auto &values = stream["values"] = nlohmann::json::array();

          for (const auto &evt : events) {
            std::string levelKey;
            switch (evt.Level) {
              case MGDF_LOG_ERROR:
                levelKey = "error";
                break;
              case MGDF_LOG_LOW:
                levelKey = "info";
                break;
              case MGDF_LOG_MEDIUM:
                levelKey = "notice";
                break;
              case MGDF_LOG_HIGH:
                levelKey = "debug";
                break;
            }
            auto &value = values.emplace_back(nlohmann::json::array());
            std::ostringstream t;
            t << evt.Timestamp * 1000000;
            value.emplace_back(t.str());
            std::ostringstream m;
            m << "lvl=" << levelKey << " sender=" << evt.Sender << " "
              << "message=\"" << evt.Message << "\"";
            value.emplace_back(m.str());
          }

          std::ostringstream requestBody;
          requestBody << root;
          remoteSender(requestBody.str(), "application/json");
        }
      }
      if (stop) {
        LoggerSection section;
        std::lock_guard<std::mutex> lock(_mutex);
        if (_events.empty()) {
          _closed = true;
          break;
        }
      }
    }
  });
}

bool Logger::MoveOutputFile() {
  LoggerSection section;
  std::lock_guard<std::mutex> write(_writeMutex);
  const auto newFile = Resources::Instance().LogFile();
  if (newFile == _filename) return true;
  std::error_code error;
  std::filesystem::remove(newFile, error);
  if (error || !std::filesystem::copy_file(_filename, newFile,
      std::filesystem::copy_options::none, error)) return false;
  std::filesystem::remove(_filename, error);
  _filename = newFile;
  return true;
}

void Logger::SetOutputFile(const std::wstring &filename) {
  _filename = filename;
  std::ofstream outFile;

  std::error_code error;
  std::filesystem::remove(_filename, error);
  if (!error) outFile.open(_filename.c_str(), std::ios::out);
  outFile.close();
}

void Logger::SetLoggingLevel(MGDFLogLevel level) { _level.store(level); }

void Logger::SetRemoteEndpoint(
    const std::string &endpoint,
    std::function<void(const std::string &content,
                       const std::string &contentType)>
        remoteSender) {
  LoggerSection section;
  std::lock_guard<std::mutex> lock(_mutex);
  _remoteEndpoint = endpoint;
  _remoteSender = remoteSender;
}

MGDFLogLevel Logger::GetLoggingLevel() const { return _level; }

void Logger::Stop() {
  if (!_flushThread.joinable()) return;
  {
    LoggerSection section;
    std::lock_guard<std::mutex> lock(_mutex);
    _runLogger = false;
    if (_hook) _hook(Point::Stop);
  }
  _cv.notify_one();
  _flushThread.join();
}

Logger::~Logger() { Stop(); }

}  // namespace core
}  // namespace MGDF
