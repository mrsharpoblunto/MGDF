#pragma once

#include <minizip/unzip.h>

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

class ZipFileWrapper {
 public:
  ZipFileWrapper(unzFile file) : _file(file) {}
  ~ZipFileWrapper() { unzClose(_file); }
  unzFile Get() { return _file; }

 private:
  unzFile _file;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF
