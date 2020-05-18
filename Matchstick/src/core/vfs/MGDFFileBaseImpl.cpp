#include "StdAfx.h"

#include "MGDFFileBaseImpl.hpp"

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

FileBaseImpl::FileBaseImpl(IFile *parent)
    : _parent(parent), _children(nullptr) {}

bool FileBaseImpl::GetParent(IFile **parent) {
  if (_parent) {
    _parent->AddRef();
    *parent = _parent;
    return true;
  } else {
    return false;
  }
}

size_t FileBaseImpl::GetChildCount() {
  if (!_children) {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_children) {
      return 0;
    }
  }
  return _children->size();
}

time_t FileBaseImpl::GetLastWriteTime() const {
  std::filesystem::path path(GetPhysicalPath());
  struct _stat64 fileInfo;
  if (_wstati64(path.c_str(), &fileInfo) != 0) {
    LOG("Unable to get last write time for "
            << Resources::ToString(GetPhysicalPath()),
        LOG_ERROR);
    return 0;
  }
  return fileInfo.st_mtime;
}

bool FileBaseImpl::GetChild(const wchar_t *name, IFile **child) {
  if (!name) {
    return false;
  }

  std::lock_guard<std::mutex> lock(_mutex);
  if (!_children) {
    return false;
  }

  auto it = _children->find(name);
  if (it != _children->end()) {
    it->second.AddRawRef(child);
    return true;
  }
  return false;
}

void FileBaseImpl::GetAllChildren(IFile **childBuffer) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_children) {
    return;
  }

  for (auto &child : *_children) {
    child.second.AddRawRef(childBuffer++);
  }
}

void FileBaseImpl::AddChild(ComObject<IFile> &file) {
  _ASSERTE(file);
  if (!_children) {
    _children = std::make_unique<
        std::map<const wchar_t *, ComObject<IFile>, WCharCmp>>();
  }
  _children->insert(std::make_pair(file->GetName(), file));
}

const wchar_t *FileBaseImpl::GetLogicalPath() {
  std::lock_guard<std::mutex> lock(_mutex);

  ComObject<IFile> parent;
  if (_logicalPath.empty() && GetParent(parent.Assign())) {
    std::vector<ComObject<IFile>> path;
    ComObject<IFile> node(this, true);
    do {
      path.push_back(node);
    } while (node->GetParent(node.Assign()));

    std::wostringstream ss;
    for (auto it = path.rbegin() + 1; it != path.rend(); ++it) {
      ss << (*it)->GetName();
      if ((*it) != this) ss << '/';
    }
    _logicalPath = ss.str();
  }

  return _logicalPath.c_str();
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
