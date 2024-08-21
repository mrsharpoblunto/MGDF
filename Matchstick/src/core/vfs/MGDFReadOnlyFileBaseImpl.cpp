#include "StdAfx.h"

#include "MGDFReadOnlyFileBaseImpl.hpp"

#include <vector>

#include "../common/MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

constexpr UINT32 BUFFER_SIZE = 1024 * 1024;

ReadOnlyFileBaseImpl::ReadOnlyFileBaseImpl(IMGDFReadOnlyFile *parent,
                                           IMGDFReadOnlyVirtualFileSystem *vfs)
    : _parent(parent), _vfs(vfs), _children(nullptr) {}

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

BOOL ReadOnlyFileBaseImpl::GetChild(const wchar_t *name,
                                    IMGDFReadOnlyFile **child) {
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

void ReadOnlyFileBaseImpl::AddChild(const ComObject<IMGDFReadOnlyFile> &file) {
  _ASSERTE(file);
  if (!_children) {
    _children = std::make_unique<
        std::map<std::wstring, ComObject<IMGDFReadOnlyFile>, WStrCmp>>();
  }
  _children->insert(std::make_pair(
      ComString<&IMGDFReadOnlyFile::GetLogicalName>(file), file));
}

HRESULT ReadOnlyFileBaseImpl::CopyTo(IMGDFWriteableFile *destination) {
  if (destination->Exists()) {
    return E_FAIL;
  }

  if (IsFolder()) {
    if (S_OK != destination->CreateFolder()) {
      return E_FAIL;
    }
    ComArray<IMGDFReadOnlyFile> children(GetChildCount());
    GetAllChildren(children.Data());
    for (auto &c : children) {
      ComObject<IMGDFWriteableFile> destChild;
      std::wstring logicalName =
          ComString<&IMGDFReadOnlyFile::GetLogicalName>(c);
      if (S_OK !=
              destination->GetChild(logicalName.c_str(), destChild.Assign()) ||
          S_OK != c->CopyTo(destChild)) {
        return E_FAIL;
      }
    }
    return S_OK;
  } else {
    std::vector<char> buffer(BUFFER_SIZE);

    ComObject<IMGDFFileReader> reader;
    if (S_OK != Open(reader.Assign())) {
      return E_FAIL;
    }
    ComObject<IMGDFFileWriter> writer;
    if (S_OK != destination->OpenWrite(writer.Assign())) {
      return E_FAIL;
    }

    auto position = reader->GetPosition();
    while (position < reader->GetSize()) {
      const auto read = reader->Read(buffer.data(), BUFFER_SIZE);
      position += read;
      if (writer->Write(buffer.data(), read) != read) {
        return E_FAIL;
      }
    }
    return S_OK;
  }
}

void ReadOnlyFileBaseImpl::GetVFS(IMGDFReadOnlyVirtualFileSystem **vfs) {
  _vfs->AddRef();
  *vfs = _vfs;
}

HRESULT ReadOnlyFileBaseImpl::GetLogicalPath(wchar_t *path, UINT64 *length) {
  if (!_logicalPath.size()) {
    const auto result = _vfs->GetLogicalPath(this, path, length);
    if (SUCCEEDED(result) && path != nullptr) {
      // cache the result
      _logicalPath.resize(*length);
      wmemcpy_s(_logicalPath.data(), _logicalPath.size(), path, *length);
    }
    return result;
  }
  return StringWriter::Write(_logicalPath, path, length);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
