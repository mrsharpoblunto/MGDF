#pragma once

#include <MGDF/MGDF.h>

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "MGDFHttpClient.hpp"

namespace MGDF {
namespace core {

#define LOG(msg, lvl) \
  MGDFLog([&](auto &oss) { oss << msg; }, lvl, __FILE__, __LINE__)

void MGDFLog(std::function<void(std::ostringstream &)> msg, MGDFLogLevel level,
             const char *file, int line);

/**
 singleton event log - uses buffered file writes to increase efficiency.
*/
class Logger {
 public:
  virtual ~Logger();
  static Logger &Instance() {
    static Logger log;
    return log;
  }

  void SetRemoteEndpoint(const std::shared_ptr<NetworkEventLoop> &eventLoop,
                         const std::string &endpoint);
  void SetLoggingLevel(MGDFLogLevel level);
  MGDFLogLevel GetLoggingLevel() const;
  void Log(const char *sender, const char *message, MGDFLogLevel level);

  void MoveOutputFile();
  void Flush();
  void FlushSync();

 private:
  Logger();

  void SetOutputFile(const std::wstring &);

  struct LogEntry {
    MGDFLogLevel Level;
    size_t Timestamp;
    std::string Sender;
    std::string Message;
  };

  std::mutex _mutex;
  std::condition_variable _cv;
  std::vector<LogEntry> _events;
  std::thread _flushThread;
  bool _runLogger;
  std::wstring _filename;
  std::atomic<MGDFLogLevel> _level;
  std::shared_ptr<HttpClient> _client;
  std::string _remoteEndpoint;
};

}  // namespace core
}  // namespace MGDF
