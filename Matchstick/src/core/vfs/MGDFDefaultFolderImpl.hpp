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
                    IFile *parent, VirtualFileSystemComponent *vfs);
  virtual ~DefaultFolderImpl(void);

  bool GetChild(const wchar_t *name, IFile **child) override final;
  size_t GetChildCount() override final;
  void GetAllChildren(IFile **childBuffer) override final;

 private:
  VirtualFileSystemComponent *_vfs;

  void MapChildren();
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF