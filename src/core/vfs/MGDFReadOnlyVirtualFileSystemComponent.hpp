#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "MGDFDefaultFileBase.hpp"
#include "MGDFVirtualFileSystemComponent.hpp"

namespace MGDF {
namespace core {
namespace vfs {

struct ReadOnlyFileContext {
  std::filesystem::path RootPath;
  std::vector<IMGDFArchiveHandler *> Handlers;

  ~ReadOnlyFileContext() {
    for (auto handler : Handlers) {
      handler->Release();
    }
  }
};

class ReadOnlyVirtualFileSystemComponent;

class DefaultReadOnlyFileImpl
    : public DefaultFileBase<IMGDFReadOnlyFile, ReadOnlyFileContext> {
 public:
  DefaultReadOnlyFileImpl(const std::wstring &name,
                          const std::filesystem::path &physicalPath,
                          std::shared_ptr<ReadOnlyFileContext> context);
  virtual ~DefaultReadOnlyFileImpl();

  BOOL __stdcall IsArchive() final { return false; }
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;

  ComObject<IMGDFReadOnlyFile> CreateFile(
      const std::wstring &name, const std::filesystem::path &path) final;
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
  size_t _rootParentPathSize;
  std::shared_ptr<ReadOnlyFileContext> _context;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF