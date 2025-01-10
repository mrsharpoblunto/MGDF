#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <filesystem>
#include <vector>

#include "MGDFLogicalPathResolver.h"
#include "MGDFVirtualFileSystemComponent.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class DefaultFolderImpl;
struct WCharCmp;

class ReadOnlyVirtualFileSystemComponent
    : public IReadOnlyVirtualFileSystemComponent {
 public:
  ReadOnlyVirtualFileSystemComponent();
  virtual ~ReadOnlyVirtualFileSystemComponent() {};

  BOOL __stdcall GetFile(const wchar_t *logicalPath,
                         IMGDFReadOnlyFile **file) final;
  void __stdcall GetRoot(IMGDFReadOnlyFile **root) final;
  HRESULT __stdcall GetLogicalPath(IMGDFReadOnlyFile *file, wchar_t *path,
                                   UINT64 *length) final;

  bool Mount(const wchar_t *physicalDirectory) final;
  void RegisterArchiveHandler(ComObject<IMGDFArchiveHandler> handler) final;
  void Map(const std::filesystem::path &path,
           ComObject<IMGDFReadOnlyFile> parent,
           ComObject<IMGDFReadOnlyFile> &child);

 private:
  std::vector<ComObject<IMGDFArchiveHandler>> _archiveHandlers;
  ComObject<IMGDFReadOnlyFile> _root;

  bool GetArchiveHandler(const std::wstring &path,
                         ComObject<IMGDFArchiveHandler> &handler);
  LogicalPathResolver<IMGDFReadOnlyFile> _resolver;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF