#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

namespace MGDF {
namespace core {
namespace vfs {

class __declspec(uuid(
    "3643D6EE-F123-415D-9137-9475A131360F")) IReadOnlyVirtualFileSystemComponent
    : public IMGDFReadOnlyVirtualFileSystem {
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
