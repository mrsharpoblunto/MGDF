#include "StdAfx.h"

#include "MGDFDefaultFolderImpl.hpp"

#include <algorithm>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

DefaultFolderImpl::DefaultFolderImpl(const std::wstring &name,
                                     const std::wstring &physicalPath,
                                     IFile *parent,
                                     VirtualFileSystemComponent *vfs)
    : FolderBaseImpl(name, physicalPath, parent), _vfs(vfs) {
  _ASSERTE(vfs);
}

DefaultFolderImpl::~DefaultFolderImpl() {}

// used by folders to lazily enumerate thier children as needed.
void DefaultFolderImpl::MapChildren() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_children) {
    _children = std::make_unique<
        std::map<const wchar_t *, ComObject<IFile>, WCharCmp>>();

    path path(GetPhysicalPath());
    _ASSERTE(is_directory(path));

    ComObject<IFile> parent(this, true);
    for (auto &p : directory_iterator(path)) {
      ComObject<IFile> mappedChild;
      _vfs->Map(p, parent, mappedChild);
      _ASSERTE(mappedChild);
      _children->insert(
          std::make_pair(mappedChild->GetName(), std::move(mappedChild)));
    }
  }
}

bool DefaultFolderImpl::GetChild(const wchar_t *name, IFile **child) {
  if (!name) return false;

  MapChildren();
  return FolderBaseImpl::GetChild(name, child);
}

size_t DefaultFolderImpl::GetChildCount() {
  MapChildren();
  return FolderBaseImpl::GetChildCount();
}

void DefaultFolderImpl::GetAllChildren(IFile **childBuffer) {
  MapChildren();
  return FolderBaseImpl::GetAllChildren(childBuffer);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF