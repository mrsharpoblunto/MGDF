#pragma once

#include <MGDF/MGDF.h>
#include <atomic>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace MGDF {
namespace core {

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

#define LOG(msg, lvl)                                                        \
  {                                                                          \
    if (lvl <= MGDF::core::Logger::Instance().GetLoggingLevel()) {           \
      std::ostringstream ss;                                                 \
      ss << __FILE__ << ':' << __LINE__;                                     \
      std::ostringstream ms;                                                 \
      ms << msg;                                                             \
      MGDF::core::Logger::Instance().Log(ss.str().c_str(), ms.str().c_str(), \
                                         lvl);                               \
    }                                                                        \
  }

}  // namespace core
}  // namespace MGDF
