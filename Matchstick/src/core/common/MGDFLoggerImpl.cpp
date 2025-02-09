#include "stdafx.h"

#include "MGDFLoggerImpl.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
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

void Logger::Log(const char *sender, const char *message, MGDFLogLevel level) {
  _ASSERTE(sender);
  _ASSERTE(message);

  if (level <= _level.load()) {
#if defined(_DEBUG)
    std::ostringstream stream;
    stream << sender << ": " << message << "\n";
    OutputDebugString(stream.str().c_str());
#endif
    {
      std::unique_lock<std::mutex> lock(_mutex);
      const auto timestamp = GetTimeStamp();
      _events.push_back(LogEntry{.Level = level,
                                 .Timestamp = timestamp,
                                 .Sender = sender,
                                 .Message = message});
      if (_events.size() >= LOG_BUFFER_SIZE) {
        lock.unlock();
        _cv.notify_one();
      }
    }
  }
}

void Logger::Flush() { _cv.notify_one(); }

void Logger::FlushSync() {
  std::unique_lock<std::mutex> lock(_mutex);
  std::vector<LogEntry> events(std::move(_events));
  _events.clear();
  lock.unlock();

  std::ofstream outFile;
  outFile.open(_filename.c_str(), std::ios::app);
  for (const auto &evt : events) {
    outFile << evt.Sender << ": " << evt.Message << std::endl;
  }
  outFile.close();
}

Logger::Logger() {
  SetLoggingLevel(MGDF_LOG_MEDIUM);
  SetOutputFile(Resources::Instance().LogFile());

  _runLogger = true;
  _flushThread = std::thread([this]() {
    while (_runLogger) {
      std::unique_lock<std::mutex> lock(_mutex);
      _cv.wait_for(lock, LOG_FLUSH_TIMEOUT,
                   [this] { return !_runLogger || _events.size() > 0; });
      std::vector<LogEntry> events(std::move(_events));
      _events.clear();
      lock.unlock();

      if (events.size()) {
        std::ofstream outFile;
        outFile.open(_filename.c_str(), std::ios::app);
        for (const auto &evt : events) {
          outFile << evt.Sender << ": " << evt.Message << std::endl;
        }

        if (_remoteEndpoint.size()) {
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
          _remoteSender(requestBody.str(), "application/json");
        }
      }
    }
  });
}

void Logger::MoveOutputFile() {
  std::wstring newFile = Resources::Instance().LogFile();
  if (newFile != _filename && _filename.size() &&
      std::filesystem::exists(_filename)) {
    std::filesystem::path from(_filename);
    std::filesystem::path to(Resources::Instance().LogFile());
    std::filesystem::copy_file(
        from, to, std::filesystem::copy_options::overwrite_existing);
    std::filesystem::remove(from);
  }
  _filename = newFile;
}

void Logger::SetOutputFile(const std::wstring &filename) {
  _filename = filename;
  std::ofstream outFile;

  outFile.open(_filename.c_str(), std::ios::out);
  outFile.close();
}

void Logger::SetLoggingLevel(MGDFLogLevel level) { _level.store(level); }

void Logger::SetRemoteEndpoint(
    const std::string &endpoint,
    std::function<void(const std::string &content,
                       const std::string &contentType)>
        remoteSender) {
  std::lock_guard<std::mutex> lock(_mutex);
  _remoteEndpoint = endpoint;
  _remoteSender = remoteSender;
}

MGDFLogLevel Logger::GetLoggingLevel() const { return _level; }

Logger::~Logger(void) {
  {
    // force whatever is in the events buffer to be flushed & stop
    // the flush thread running after its finished this last flush
    std::unique_lock<std::mutex> lock(_mutex);
    _runLogger = false;
  }
  _cv.notify_one();
  _flushThread.join();
}

}  // namespace core
}  // namespace MGDF
