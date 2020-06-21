#pragma once

#include <filesystem>

#include "MGDFFolderBaseImpl.hpp"
#include "MGDFVirtualFileSystemComponentImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

/**
folders map thier children lazily to speed up initial vfs enumeration on program
startup, this means that when a request is made for the children of a folder, it
has to call back into the vfs component to enumerate its children at that time.
*/
class DefaultFolderImpl : public FolderBaseImpl {
 public:
  DefaultFolderImpl(const std::wstring &name, const std::wstring &physicalPath,
                    IMGDFFile *parent, VirtualFileSystemComponent *vfs);
  virtual ~DefaultFolderImpl(void);

  BOOL __stdcall GetChild(const wchar_t *name, IMGDFFile **child) final;
  UINT64 __stdcall GetChildCount() final;
  void __stdcall GetAllChildren(IMGDFFile **childBuffer) final;

 private:
  VirtualFileSystemComponent *_vfs;

  void MapChildren();
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF