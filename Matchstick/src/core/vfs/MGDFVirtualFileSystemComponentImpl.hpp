#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <filesystem>
#include <map>
#include <vector>

namespace MGDF {
namespace core {
namespace vfs {

class IVirtualFileSystemComponent : public ComBase<IVirtualFileSystem> {
 public:
  virtual ~IVirtualFileSystemComponent() {}
  virtual bool Mount(const wchar_t *physicalDirectory) = 0;
  virtual void RegisterArchiveHandler(ComObject<IArchiveHandler> handler) = 0;
};

class DefaultFolderImpl;
struct WCharCmp;

class VirtualFileSystemComponent : public IVirtualFileSystemComponent {
 public:
  VirtualFileSystemComponent();
  virtual ~VirtualFileSystemComponent(){};

  bool GetFile(const wchar_t *logicalPath, IFile **file) final;
  void GetRoot(IFile **root) final;
  bool Mount(const wchar_t *physicalDirectory) final;
  void RegisterArchiveHandler(ComObject<IArchiveHandler> handler) final;

  void Map(const std::filesystem::path &path, ComObject<IFile> parent,
           ComObject<IFile> &child);

 private:
  std::vector<ComObject<IArchiveHandler>> _archiveHandlers;
  ComObject<IFile> _root;

  bool GetArchiveHandler(const std::wstring &path,
                         ComObject<IArchiveHandler> &handler);
};

bool CreateVirtualFileSystemComponentImpl(
    ComObject<IVirtualFileSystemComponent> &vfs);

}  // namespace vfs
}  // namespace core
}  // namespace MGDF