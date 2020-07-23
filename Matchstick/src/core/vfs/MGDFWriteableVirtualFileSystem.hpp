#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <filesystem>
#include <map>
#include <mutex>
#include <vector>

#include "MGDFLogicalPathResolver.h"

namespace MGDF {
namespace core {
namespace vfs {

class WriteableVirtualFileSystem
    : public ComBase<IMGDFWriteableVirtualFileSystem> {
 public:
  virtual ~WriteableVirtualFileSystem() {}
  WriteableVirtualFileSystem(const std::wstring &rootPath)
      : _rootPath(rootPath) {}

  BOOL __stdcall GetFile(const wchar_t *logicalPath,
                         IMGDFWriteableFile **file) final;
  void __stdcall GetRoot(IMGDFWriteableFile **root) final;

  HRESULT __stdcall GetLogicalPath(IMGDFWriteableFile *file, wchar_t *path,
                                   UINT64 *length) final;

  const std::filesystem::path &GetRootPath() { return _rootPath; }

 private:
  std::filesystem::path _rootPath;
  LogicalPathResolver<IMGDFWriteableFile> _resolver;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
