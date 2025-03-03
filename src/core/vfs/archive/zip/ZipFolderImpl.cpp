#include "StdAfx.h"

#include "ZipFolderImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

void ZipFolderImpl::AddChild(std::shared_ptr<ZipResource> child) {
  _children.insert(std::make_pair(child->GetName(), child.get()));
  _context->Resources.push_back(child);
}

HRESULT ZipFolderImpl::Open(IMGDFFileReader **reader) {
  std::ignore = reader;
  return E_FAIL;
}

HRESULT ZipFolderImpl::CopyTo(IMGDFWriteableFile *destination) {
  if (S_OK != destination->CreateFolder()) {
    return E_FAIL;
  }
  size_t childCount = 0;
  GetAllChildren(nullptr, &childCount);
  ComArray<IMGDFReadOnlyFile> children(childCount);
  if (FAILED(GetAllChildren(children.Data(), &childCount))) {
    return E_FAIL;
  }

  for (auto &c : children) {
    ComObject<IMGDFWriteableFile> destChild;
    std::wstring logicalName = ComString<&IMGDFReadOnlyFile::GetLogicalName>(c);
    if (!destination->GetChild(logicalName.c_str(), destChild.Assign()) ||
        S_OK != c->CopyTo(destChild)) {
      return E_FAIL;
    }
  }
  return S_OK;
}

HRESULT ZipFolderImpl::GetAllChildren(IMGDFReadOnlyFile **buffer,
                                      UINT64 *length) {
  const auto inputLength = *length;
  *length = _children.size();
  if (!buffer) {
    return S_OK;
  } else if (inputLength < _children.size()) {
    return E_NOT_SUFFICIENT_BUFFER;
  }

  for (auto &c : _children) {
    c.second->AddRef();
    *(buffer++) = c.second;
  }
  return S_OK;
}

UINT64 ZipFolderImpl::GetChildCount() { return _children.size(); }

BOOL ZipFolderImpl::GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) {
  const auto found = _children.find(name);
  if (found != _children.end()) {
    found->second->AddRef();
    *child = found->second;
    return TRUE;
  } else {
    return FALSE;
  }
}

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF
