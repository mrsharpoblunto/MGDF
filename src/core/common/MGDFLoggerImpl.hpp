#pragma once

#include <MGDF/MGDF.h>

#include <atomic>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

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
  enum class Point { Append, Write, Stop };
  explicit Logger(const std::wstring &filename,
                  std::function<void(Point)> hook = {});
  virtual ~Logger();
  void Stop();
  bool LogCrash(const char *sender, const char *message);
  static bool IsLockedByThisThread();
  static Logger &Instance() {
    static Logger log;
    return log;
  }

  void SetRemoteEndpoint(const std::string &endpoint,
                         std::function<void(const std::string &content,
                                            const std::string &contentType)>
                             remoteSender);
  void SetLoggingLevel(MGDFLogLevel level);
  MGDFLogLevel GetLoggingLevel() const;
  void Log(const char *sender, const char *message, MGDFLogLevel level);

  bool MoveOutputFile();
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

  void Write(const std::vector<LogEntry> &events);
  std::function<void(Point)> _hook;
  std::mutex _writeMutex;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::vector<LogEntry> _events;
  std::thread _flushThread;
  bool _runLogger;
  bool _closed = false;
  std::wstring _filename;
  std::atomic<MGDFLogLevel> _level;
  std::function<void(const std::string &content,
                     const std::string &contentType)>
      _remoteSender;
  std::string _remoteEndpoint;
};

}  // namespace core
}  // namespace MGDF
