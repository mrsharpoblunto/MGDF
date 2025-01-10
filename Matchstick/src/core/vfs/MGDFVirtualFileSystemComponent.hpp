#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

namespace MGDF {
namespace core {
namespace vfs {

class IReadOnlyVirtualFileSystemComponent
    : public MGDF::ComBase<IMGDFReadOnlyVirtualFileSystem> {
 public:
  virtual ~IReadOnlyVirtualFileSystemComponent() {}
  virtual bool Mount(const wchar_t *physicalDirectory) = 0;
  virtual void RegisterArchiveHandler(
      MGDF::ComObject<IMGDFArchiveHandler> handler) = 0;
};

bool CreateReadOnlyVirtualFileSystemComponent(
    MGDF::ComObject<IReadOnlyVirtualFileSystemComponent> &vfs);

bool CreateWriteableVirtualFileSystemComponent(
    const std::wstring &rootDirectory,
    MGDF::ComObject<IMGDFWriteableVirtualFileSystem> &vfs);

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
