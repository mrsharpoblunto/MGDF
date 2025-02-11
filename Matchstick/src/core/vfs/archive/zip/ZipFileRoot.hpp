#pragma once

#include <minizip/unzip.h>

#include "../../MGDFDefaultReadOnlyFileImpl.hpp"
#include "ZipCommon.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
 this class acts the same as a normal file in all ways except that it has a non
 empty archive name and returns true as an archive. This allows for the actual
 phsyical zip file to be accessed like any other file, but it also allows for
 accessing the contents of the zip file as if it were a folder
 */
class ZipFileRoot : public DefaultReadOnlyFileImpl {
 public:
  ZipFileRoot(const std::wstring &name, const std::wstring &physicalPath,
              IMGDFReadOnlyFile *parent, IMGDFReadOnlyVirtualFileSystem *vfs,
              std::shared_ptr<ZipFileWrapper> zip)
      : DefaultReadOnlyFileImpl(name, physicalPath, parent, vfs), _zip(zip) {}
  ~ZipFileRoot() {}

  BOOL __stdcall IsArchive() final { return true; }

 private:
  std::shared_ptr<ZipFileWrapper> _zip;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF