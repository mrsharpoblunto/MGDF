#include "stdafx.h"

#include "MGDFLoggerImpl.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

#define LOG_BUFFER_SIZE 10

void MGDFLog(std::function<void(std::ostringstream &)> msg, MGDFLogLevel level,
             const char *file, int line) {
  if (level <= Logger::Instance().GetLoggingLevel()) {
    std::ostringstream ss;
    ss << file << ':' << line;
    std::ostringstream ms;
    msg(ms);
    Logger::Instance().Log(ss.str().c_str(), ms.str().c_str(), level);
  }
}

void Logger::Log(const char *sender, const char *message, MGDFLogLevel level) {
  _ASSERTE(sender);
  _ASSERTE(message);

  if (level <= _level.load()) {
    std::ostringstream stream;
    stream << sender << " " << message << "\n";
#if defined(_DEBUG)
    OutputDebugString(("MGDF: " + stream.str()).c_str());
#endif
    {
      std::unique_lock<std::mutex> lock(_mutex);
      _events.push_back(stream.str());
      if (_events.size() >= LOG_BUFFER_SIZE) {
        // add a batch of events to flush & notify the flush thread
        _flushEvents.assign(_events.begin(), _events.end());
        _events.clear();
        lock.unlock();
        _cv.notify_one();
      }
    }
  }
}

void Logger::Flush() {
  {
    // force whatever is in the events buffer to be flushed
    std::lock_guard<std::mutex> lock(_mutex);
    _flushEvents.assign(_events.begin(), _events.end());
    _events.clear();
  }
  _cv.notify_one();
}

Logger::Logger() {
  SetLoggingLevel(MGDF_LOG_MEDIUM);
  SetOutputFile(Resources::Instance().LogFile());

  _runLogger = true;
  _flushThread = std::thread([this]() {
    std::unique_lock<std::mutex> lock(_mutex);
    while (_runLogger) {
      _cv.wait(lock, [this] { return !_runLogger || _flushEvents.size() > 0; });
      std::vector<std::string> tmp(_flushEvents);
      _flushEvents.clear();
      lock.unlock();

      std::ofstream outFile;
      outFile.open(_filename.c_str(), std::ios::app);
      for (const std::string &evt : tmp) {
        outFile << evt;
      }
      outFile.close();

      lock.lock();
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

MGDFLogLevel Logger::GetLoggingLevel() const { return _level; }

Logger::~Logger(void) {
  {
    // force whatever is in the events buffer to be flushed & stop
    // the flush thread running after its finished this last flush
    std::unique_lock<std::mutex> lock(_mutex);
    _flushEvents.assign(_events.begin(), _events.end());
    _events.clear();
    _runLogger = false;
  }
  _cv.notify_one();
  _flushThread.join();
}

}  // namespace core
}  // namespace MGDF
