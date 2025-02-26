#include "StdAfx.h"

#include "ZipArchive.hpp"

#include <sstream>

#include "../../../common/MGDFStringImpl.hpp"
#include "../../MGDFReadOnlyVirtualFileSystemComponent.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

ULONG __stdcall ZipResource::AddRef() { return _archive->AddRef(); };
ULONG __stdcall ZipResource::Release() { return _archive->Release(); }
HRESULT __stdcall ZipResource::QueryInterface(REFIID riid, void **ppvObject) {
  if (!ppvObject) return E_POINTER;
  if (IIDMatch<IMGDFReadOnlyFile, IMGDFFile>(riid)) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  }
  return E_NOINTERFACE;
}

HRESULT ZipResource::GetLogicalName(wchar_t *name, UINT64 *length) {
  return StringWriter::Write(_name, name, length);
}

HRESULT ZipResource::GetPhysicalName(wchar_t *name, UINT64 *length) {
  return StringWriter::Write(_archive->GetPhysicalPath().filename().wstring(),
                             name, length);
}

HRESULT ZipResource::GetPhysicalPath(wchar_t *path, UINT64 *length) {
  return StringWriter::Write(_archive->GetPhysicalPath().wstring(), path,
                             length);
}

HRESULT ZipResource::GetLogicalPath(wchar_t *path, UINT64 *length) {
  std::vector<ZipResource *> components;
  components.push_back(this);
  ZipResource *parent = _parent;
  while (parent) {
    components.push_back(parent);
    parent = parent->_parent;
  }

  size_t logicalPathLength = _archive->GetLogicalPath().length();
  for (const auto &c : components) {
    logicalPathLength += c->GetName().length() + 1;
  }
  const auto inputLength = *length;
  *length = logicalPathLength;

  if (!path) {
    return S_OK;
  } else if (inputLength < logicalPathLength) {
    return E_NOT_SUFFICIENT_BUFFER;
  }

  std::wostringstream ss;
  ss << _archive->GetLogicalPath();
  for (const auto &c : components) {
    ss << L'/' << c->GetName();
  }
  const auto logicalPath = ss.str();
  memcpy_s(path, logicalPathLength * sizeof(wchar_t), logicalPath.c_str(),
           logicalPath.size() * sizeof(wchar_t));
  return S_OK;
}

BOOL ZipResource::GetParent(IMGDFReadOnlyFile **parent) {
  if (_parent) {
    _parent->AddRef();
    *parent = _parent;
    return true;
  } else {
    auto parentPath = _archive->GetPhysicalPath().parent_path();
    auto parentName = parentPath.filename();
    auto parentFile = MakeCom<DefaultReadOnlyFileImpl>(
        parentName.wstring(), parentPath, _archive->GetPhysicalPath());
    parentFile.AddRawRef(parent);
    return true;
  }
}

UINT64 ZipResource::GetLastWriteTime() { return _archive->GetLastWriteTime(); }

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF
