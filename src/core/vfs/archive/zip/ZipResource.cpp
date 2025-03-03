#include "StdAfx.h"

#include "ZipResource.hpp"

#include <sstream>

#include "../../../common/MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

ZipResource::ZipResource(const wchar_t *name, ZipResource *parent,
                         ZipContext *context)
    : _name(name), _parent(parent), _context(context) {}

ULONG ZipResource::AddRef() { return _context->AddRef(); };
ULONG ZipResource::Release() { return _context->Release(); }
HRESULT ZipResource::QueryInterface(REFIID riid, void **ppvObject) {
  if (!ppvObject) return E_POINTER;
  if (IIDMatch<IMGDFReadOnlyFile, IMGDFFile>(riid)) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  }
  return E_NOINTERFACE;
};

HRESULT ZipResource::GetLogicalName(wchar_t *name, UINT64 *length) {
  return StringWriter::Write(_name, name, length);
}

HRESULT ZipResource::GetPhysicalName(wchar_t *name, UINT64 *length) {
  return StringWriter::Write(_context->PhysicalPath.filename().wstring(), name,
                             length);
}

HRESULT ZipResource::GetPhysicalPath(wchar_t *path, UINT64 *length) {
  return StringWriter::Write(_context->PhysicalPath.wstring(), path, length);
}

HRESULT ZipResource::GetLogicalPath(wchar_t *path, UINT64 *length) {
  std::vector<ZipResource *> components;
  components.push_back(this);
  ZipResource *parent = _parent;
  while (parent) {
    components.push_back(parent);
    parent = parent->_parent;
  }

  size_t logicalPathLength = _context->LogicalPath.length();
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
  ss << _context->LogicalPath;
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
    auto archiveParent = _context->Parent;
    if (archiveParent) {
      archiveParent.AddRawRef(parent);
      return true;
    }
  }
  return false;
}

UINT64 ZipResource::GetLastWriteTime() { return _context->LastWriteTime; }

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF
