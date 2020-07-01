#include "StdAfx.h"

#include "MGDFDefaultReadOnlyFolderImpl.hpp"

#include <algorithm>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

DefaultReadOnlyFolderImpl::DefaultReadOnlyFolderImpl(const std::wstring &name,
                                     const std::wstring &physicalPath,
                                     IMGDFReadOnlyFile *parent,
                                     ReadOnlyVirtualFileSystemComponent *vfs)
    : ReadOnlyFolderBaseImpl(name, physicalPath, parent), _vfs(vfs) {
  _ASSERTE(vfs);
}

DefaultReadOnlyFolderImpl::~DefaultReadOnlyFolderImpl() {}

// used by folders to lazily enumerate thier children as needed.
void DefaultReadOnlyFolderImpl::MapChildren() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_children) {
    _children = std::make_unique<
        std::map<const wchar_t *, ComObject<IMGDFReadOnlyFile>, WCharCmp>>();

    path path(GetPhysicalPath());
    _ASSERTE(is_directory(path));

    ComObject<IMGDFReadOnlyFile> parent(this, true);
    for (auto &p : directory_iterator(path)) {
      ComObject<IMGDFReadOnlyFile> mappedChild;
      _vfs->Map(p, parent, mappedChild);
      _ASSERTE(mappedChild);
      _children->insert(
          std::make_pair(mappedChild->GetName(), std::move(mappedChild)));
    }
  }
}

BOOL DefaultReadOnlyFolderImpl::GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) {
  if (!name) return false;

  MapChildren();
  return ReadOnlyFolderBaseImpl::GetChild(name, child);
}

UINT64 DefaultReadOnlyFolderImpl::GetChildCount() {
  MapChildren();
  return ReadOnlyFolderBaseImpl::GetChildCount();
}

void DefaultReadOnlyFolderImpl::GetAllChildren(IMGDFReadOnlyFile **childBuffer) {
  MapChildren();
  return ReadOnlyFolderBaseImpl::GetAllChildren(childBuffer);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF