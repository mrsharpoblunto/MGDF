#pragma once

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "mongoose.h"

namespace MGDF {
namespace core {

// mongoose has a virtual filsystem, so this is an in-memory
// implementation to populate with SSL certs as needed
class MemFS {
 public:
  static void Ensure(
      const std::string &file,
      std::function<
          void(std::function<void(const std::string &, const std::string &)>)>);
  static void InitMGFS(mg_fs &fs);

 private:
  struct FD {
    std::string &Content;
    size_t Offset;
  };
  static std::unordered_map<std::string, std::string> _content;
  static std::mutex _mutex;

  static int st(const char *path, size_t *size, time_t *mtime);
  static void *op(const char *path, int flags);
  static void cl(void *fd);
  static size_t rd(void *fd, void *buf, size_t len);  // Read file
};

}  // namespace core
}  // namespace MGDF
