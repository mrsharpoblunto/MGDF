#pragma once

#include "../../MGDFFolderBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
implementation of a folder in a zipped archive
*/
class ZipFolderImpl : public FolderBaseImpl {
 public:
  ZipFolderImpl(const wchar_t *name, IMGDFFile *parent, IMGDFFile *root)
      : FolderBaseImpl(name, root->GetPhysicalPath(), parent), _root(root) {}
  ~ZipFolderImpl(){};

  BOOL __stdcall IsArchive() final { return true; }
  const wchar_t * __stdcall GetArchiveName() final { return _root->GetName(); }
  UINT64 __stdcall GetLastWriteTime() final { return _root->GetLastWriteTime(); }

 private:
  IMGDFFile *_root;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF