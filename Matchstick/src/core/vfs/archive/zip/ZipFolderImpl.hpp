#pragma once

#include "../../MGDFFolderBaseImpl.hpp"
#include "ZipArchive.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
implementation of a folder in a zipped archive
these files are essentially flyweight objects, all the zip functionality is
encapsulated in the ZipArchiveHandler class
*/
class ZipFolderImpl : public FolderBaseImpl {
 public:
  ZipFolderImpl(const wchar_t *name, IFile *parent, ZipArchive *handler)
      : FolderBaseImpl(name, handler->GetArchiveRoot()->GetPhysicalPath(),
                       parent),
        _handler(handler) {}
  virtual ~ZipFolderImpl();

  bool IsArchive() const override final { return true; }

  const wchar_t *GetArchiveName() const override final {
    return _handler->GetArchiveRoot()->GetName();
  }

  time_t GetLastWriteTime() const override final {
    return _handler->GetArchiveRoot()->GetLastWriteTime();
  }

 private:
  ZipArchive *_handler;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF