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
  virtual ~Logger();
  static Logger &Instance() {
    static Logger log;
    return log;
  }

  void SetLoggingLevel(MGDFLogLevel level);
  MGDFLogLevel GetLoggingLevel() const;
  void Log(const char *sender, const char *message, MGDFLogLevel level);

  void MoveOutputFile();
  void Flush();

 private:
  Logger();

  void SetOutputFile(const std::wstring &);

  std::mutex _mutex;
  std::condition_variable _cv;
  std::vector<std::string> _events;
  std::vector<std::string> _flushEvents;
  std::thread _flushThread;
  bool _runLogger;
  std::wstring _filename;
  std::atomic<MGDFLogLevel> _level;
};

}  // namespace core
}  // namespace MGDF
