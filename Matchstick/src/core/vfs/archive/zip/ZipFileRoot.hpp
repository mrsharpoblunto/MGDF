#pragma once

#include <unzip.h>

#include "../../MGDFDefaultFileImpl.hpp"

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
class ZipFileRoot : public DefaultFileImpl {
 public:
  ZipFileRoot(const std::wstring &name, const std::wstring &physicalPath,
              IFile *parent, unzFile zip)
      : DefaultFileImpl(name, physicalPath, parent),
        _archiveName(name),
        _zip(zip) {}
  ~ZipFileRoot();
  bool IsArchive() const final { return true; }
  const wchar_t *GetArchiveName() const final { return _archiveName.c_str(); }

 private:
  std::wstring _archiveName;
  unzFile _zip;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF