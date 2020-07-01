#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <filesystem>
#include <map>
#include <vector>

namespace MGDF {
namespace core {
namespace vfs {

class WriteableVirtualFileSystem
    : public ComBase<IMGDFWriteableVirtualFileSystem> {
 public:
  virtual ~WriteableVirtualFileSystem(){}
  WriteableVirtualFileSystem(const std::wstring &rootPath) : _rootPath(rootPath) {}

  BOOL __stdcall GetFile(const wchar_t *logicalPath,
                         IMGDFWriteableFile **file) final;
  void __stdcall GetRoot(IMGDFWriteableFile **root) final;

 private:
  std::filesystem::path _rootPath;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
