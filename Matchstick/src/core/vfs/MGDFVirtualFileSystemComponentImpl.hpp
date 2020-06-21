#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
#include <filesystem>
#include <map>
#include <vector>

namespace MGDF {
namespace core {
namespace vfs {

class IVirtualFileSystemComponent : public ComBase<IMGDFVirtualFileSystem> {
 public:
  virtual ~IVirtualFileSystemComponent() {}
  virtual bool Mount(const wchar_t *physicalDirectory) = 0;
  virtual void RegisterArchiveHandler(ComObject<IMGDFArchiveHandler> handler) = 0;
};

class DefaultFolderImpl;
struct WCharCmp;

class VirtualFileSystemComponent : public IVirtualFileSystemComponent {
 public:
  VirtualFileSystemComponent();
  virtual ~VirtualFileSystemComponent(){};

  BOOL __stdcall GetFile(const wchar_t *logicalPath, IMGDFFile **file) final;
  void __stdcall GetRoot(IMGDFFile **root) final;

  bool Mount(const wchar_t *physicalDirectory) final;
  void RegisterArchiveHandler(ComObject<IMGDFArchiveHandler> handler) final;
  void Map(const std::filesystem::path &path, ComObject<IMGDFFile> parent,
           ComObject<IMGDFFile> &child);

 private:
  std::vector<ComObject<IMGDFArchiveHandler>> _archiveHandlers;
  ComObject<IMGDFFile> _root;

  bool GetArchiveHandler(const std::wstring &path,
                         ComObject<IMGDFArchiveHandler> &handler);
};

bool CreateVirtualFileSystemComponentImpl(
    ComObject<IVirtualFileSystemComponent> &vfs);

}  // namespace vfs
}  // namespace core
}  // namespace MGDF