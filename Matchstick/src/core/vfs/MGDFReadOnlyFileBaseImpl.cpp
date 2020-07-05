#include "StdAfx.h"

#include "MGDFReadOnlyFileBaseImpl.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <filesystem>
#include <sstream>
#include <vector>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

ReadOnlyFileBaseImpl::ReadOnlyFileBaseImpl(IMGDFReadOnlyFile *parent)
    : _parent(parent), _children(nullptr) {}

BOOL ReadOnlyFileBaseImpl::GetParent(IMGDFReadOnlyFile **parent) {
  if (_parent) {
    _parent->AddRef();
    *parent = _parent;
    return true;
  } else {
    return false;
  }
}

size_t ReadOnlyFileBaseImpl::GetChildCount() {
  if (!_children) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_children) {
      return 0;
    }
  }
  return _children->size();
}

UINT64 ReadOnlyFileBaseImpl::GetLastWriteTime() {
  std::filesystem::path path(GetPhysicalPath());
  struct _stat64 fileInfo;
  if (_wstati64(path.c_str(), &fileInfo) != 0) {
    LOG("Unable to get last write time for "
            << Resources::ToString(GetPhysicalPath()),
        MGDF_LOG_ERROR);
    return 0;
  }
  return fileInfo.st_mtime;
}

BOOL ReadOnlyFileBaseImpl::GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) {
  if (!name) {
    return false;
  }

  std::lock_guard<std::mutex> lock(_mutex);
  if (!_children) {
    return false;
  }

  const auto it = _children->find(name);
  if (it != _children->end()) {
    it->second.AddRawRef(child);
    return true;
  }
  return false;
}

void ReadOnlyFileBaseImpl::GetAllChildren(IMGDFReadOnlyFile **childBuffer) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_children) {
    return;
  }

  for (auto &child : *_children) {
    child.second.AddRawRef(childBuffer++);
  }
}

void ReadOnlyFileBaseImpl::AddChild(ComObject<IMGDFReadOnlyFile> &file) {
  _ASSERTE(file);
  if (!_children) {
    _children = std::make_unique<
        std::map<const wchar_t *, ComObject<IMGDFReadOnlyFile>, WCharCmp>>();
  }
  _children->insert(std::make_pair(file->GetName(), file));
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
