#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "MGDFDefaultFileBase.hpp"
#include "MGDFVirtualFileSystemComponent.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class ReadOnlyVirtualFileSystemComponent;

class DefaultReadOnlyFileImpl : public DefaultFileBase<IMGDFReadOnlyFile> {
 public:
  DefaultReadOnlyFileImpl(const std::wstring &name,
                          const std::filesystem::path &physicalPath,
                          const std::filesystem::path &rootPath,
                          IMGDFArchiveHandler **handlers, size_t handlerCount);
  virtual ~DefaultReadOnlyFileImpl();

  BOOL __stdcall IsArchive() final { return false; }
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;

  ComObject<IMGDFReadOnlyFile> CreateFile(
      const std::wstring &name, const std::filesystem::path &path,
      const std::filesystem::path &rootPath) final;

 private:
  IMGDFArchiveHandler **_handlers;
  UINT64 _handlerCount;
};

class ReadOnlyVirtualFileSystemComponent
    : public ComBase<IReadOnlyVirtualFileSystemComponent,
                     IMGDFReadOnlyVirtualFileSystem> {
  friend class DefaultReadOnlyFileImpl;

 public:
  ReadOnlyVirtualFileSystemComponent();
  virtual ~ReadOnlyVirtualFileSystemComponent();

  BOOL __stdcall GetFile(const wchar_t *logicalPath,
                         IMGDFReadOnlyFile **file) final;
  void __stdcall GetRoot(IMGDFReadOnlyFile **root) final;

  bool Mount(const wchar_t *physicalDirectory) final;
  void RegisterArchiveHandler(ComObject<IMGDFArchiveHandler> handler) final;

 private:
  BOOL Get(const std::filesystem::path &path, IMGDFReadOnlyFile **file);
  std::filesystem::path _rootPath;
  size_t _rootParentPathSize;
  std::vector<IMGDFArchiveHandler *> _archiveHandlers;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF