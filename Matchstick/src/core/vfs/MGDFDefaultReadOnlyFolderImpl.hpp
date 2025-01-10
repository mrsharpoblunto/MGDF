#pragma once

#include <filesystem>

#include "MGDFDefaultReadOnlyFileImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

/**
folders map thier children lazily to speed up initial vfs enumeration on program
startup, this means that when a request is made for the children of a folder, it
has to call back into the vfs component to enumerate its children at that time.
*/
class DefaultReadOnlyFolderImpl : public DefaultReadOnlyFileImpl {
 public:
  DefaultReadOnlyFolderImpl(const std::wstring &name,
                            const std::wstring &physicalPath,
                            IMGDFReadOnlyFile *parent,
                            IMGDFReadOnlyVirtualFileSystem *vfs);
  virtual ~DefaultReadOnlyFolderImpl(void);

  BOOL __stdcall IsFolder() final { return true; }
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;
  UINT64 __stdcall GetChildCount() final;
  void __stdcall GetAllChildren(IMGDFReadOnlyFile **childBuffer) final;

 private:
  void MapChildren();
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF