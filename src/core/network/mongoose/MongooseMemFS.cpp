#include "stdafx.h"

#include "MongooseMemFS.hpp"

#pragma warning(disable : 4706)

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

std::unordered_map<std::string, std::string> MemFS::_content;
std::mutex MemFS::_mutex;

void MemFS::Ensure(
    const std::string &file,
    std::function<
        void(std::function<void(const std::string &, const std::string &)>)>
        changes) {
  std::lock_guard lock(_mutex);
  if (!_content.contains(file)) {
    changes([](const std::string &file, const std::string &content) {
      _content.insert(std::make_pair(file, content));
    });
  }
}

int MemFS::st(const char *path, size_t *size, time_t *mtime) {
  std::ignore = mtime;
  std::lock_guard lock(_mutex);
  *size = _content.at(path).size();
  return 0;
}

void *MemFS::op(const char *path, int flags) {
  std::ignore = flags;
  std::lock_guard lock(_mutex);
  const auto found = _content.find(path);
  if (found == _content.end()) {
    return nullptr;
  } else {
    return new FD{.Content = found->second, .Offset = 0U};
  }
}

void MemFS::cl(void *fd) { delete static_cast<FD *>(fd); }

size_t MemFS::rd(void *fd, void *buf, size_t len) {
  FD *context = static_cast<FD *>(fd);
  if (context->Offset + len <= context->Content.size()) {
    memcpy(buf, context->Content.data() + context->Offset, len);
    context->Offset += len;
    return len;
  } else {
    const size_t read = context->Content.size() - len;
    if (read != 0) {
      memcpy(buf, context->Content.data() + context->Offset, read);
      context->Offset += len;
    }
    return read;
  }
}

void MemFS::InitMGFS(mg_fs &fs) {
  fs.st = &MemFS::st;
  fs.op = &MemFS::op;
  fs.rd = &MemFS::rd;
  fs.cl = &MemFS::cl;
}

}  // namespace core
}  // namespace MGDF
