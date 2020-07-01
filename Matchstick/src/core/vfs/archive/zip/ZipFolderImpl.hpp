#pragma once

#include "../../MGDFReadOnlyFolderBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
implementation of a folder in a zipped archive
*/
class ZipFolderImpl : public ReadOnlyFolderBaseImpl {
 public:
  ZipFolderImpl(const wchar_t *name, IMGDFReadOnlyFile *parent, IMGDFReadOnlyFile *root)
      : ReadOnlyFolderBaseImpl(name, root->GetPhysicalPath(), parent), _root(root) {}
  ~ZipFolderImpl(){};

  BOOL __stdcall IsArchive() final { return true; }
  const wchar_t * __stdcall GetArchiveName() final { return _root->GetName(); }
  UINT64 __stdcall GetLastWriteTime() final { return _root->GetLastWriteTime(); }

 private:
  IMGDFReadOnlyFile *_root;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF