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
  ZipFolderImpl(const wchar_t *name, IFile *parent, const IFile *root)
      : FolderBaseImpl(name, root->GetPhysicalPath(), parent), _root(root) {}
  virtual ~ZipFolderImpl();

  bool IsArchive() const override final { return true; }

  const wchar_t *GetArchiveName() const override final {
    return _root->GetName();
  }

  time_t GetLastWriteTime() const override final {
    return _root->GetLastWriteTime();
  }

 private:
  const IFile *_root;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF