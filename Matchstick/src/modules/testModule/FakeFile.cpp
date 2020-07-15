#include "StdAfx.h"

#include "FakeFile.hpp"

#include <MGDF/ComObject.hpp>
#include <algorithm>
#include <sstream>
#include <vector>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace Test {

FakeFile::FakeFile(const std::wstring &name, const std::wstring &physicalFile,
                   IMGDFReadOnlyFile *parent)
    : _parent(parent),
      _children(nullptr),
      _name(name),
      _physicalPath(physicalFile),
      _data(""),
      _isOpen(false),
      _position(0),
      _logicalPath(L""),
      _references(1UL) {}

FakeFile::FakeFile(const std::wstring &name, FakeFile *parent,
                   const std::string &data)  // nullptr data indicates a folder
    : _parent(parent),
      _children(nullptr),
      _name(name),
      _physicalPath(parent->_physicalPath),
      _data(data),
      _isOpen(false),
      _position(0),
      _logicalPath(L""),
      _references(1UL) {}

FakeFile::~FakeFile() {}

ULONG FakeFile::AddRef() { return ++_references; }

ULONG FakeFile::Release() {
  {
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_data.empty() && _isOpen) {
      _isOpen = false;
    }
  }
  const ULONG refs = --_references;
  if (refs == 0UL) {
    delete this;
  };
  return refs;
}

HRESULT FakeFile::QueryInterface(REFIID riid, void **ppvObject) {
  if (!ppvObject) return E_POINTER;
  if (riid == IID_IUnknown || riid == __uuidof(IMGDFFile) ||
      (_isOpen && riid == _uuidof(IMGDFFileReader))) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  }
  return E_NOINTERFACE;
};

BOOL FakeFile::GetParent(IMGDFReadOnlyFile **parent) {
  _parent->AddRef();
  *parent = _parent;
  return true;
}

UINT64 FakeFile::GetChildCount() {
  if (_children) {
    return _children->size();
  }
  return 0;
}

UINT64 FakeFile::GetLastWriteTime() { return 0; }

BOOL FakeFile::GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) {
  if (!_children || !name) return false;

  const auto it = _children->find(name);
  if (it != _children->end()) {
    it->second.Ref.AddRawRef(child);
    return true;
  }
  return false;
}

void FakeFile::GetAllChildren(IMGDFReadOnlyFile **childBuffer) {
  for (auto child : *_children) {
    child.second.Ref.AddRawRef(childBuffer++);
  }
}

void FakeFile::AddChild(ComObject<FakeFile> file) {
  _ASSERTE(file);
  if (!_children) {
    _children = std::make_unique<
        std::map<const wchar_t *, FakeFile::ChildFileRef, WCharCmp>>();
  }
  ChildFileRef ref;
  UINT64 length = 0;
  file->GetLogicalName(nullptr, &length);
  ref.Name.resize(length);
  file->GetLogicalName(ref.Name.data(), &length);
  ref.Ref = std::move(file);

  _children->insert(std::make_pair(ref.Name.data(), std::move(ref)));
}

BOOL FakeFile::IsOpen() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _isOpen;
}

HRESULT FakeFile::Open(IMGDFFileReader **reader) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_data.empty() && !_isOpen) {
    _isOpen = true;
    _position = 0;
    this->AddRef();
    *reader = this;
    return S_OK;
  }
  return E_FAIL;
}

UINT32 FakeFile::Read(void *buffer, UINT32 length) {
  if (_isOpen) {
    const INT32 oldPosition = _position;
    if ((static_cast<size_t>(oldPosition) + length) > _data.size())
      length = static_cast<INT32>(_data.size()) - oldPosition;
    memcpy(buffer, &((char *)_data.data())[oldPosition], length);
    _position = oldPosition + static_cast<INT32>(length);
    return _position;
  }
  return 0;
}

void FakeFile::SetPosition(INT64 pos) {
  if (_isOpen) {
    _position = static_cast<INT32>(pos);
  }
}

INT64 FakeFile::GetPosition() {
  if (_isOpen) {
    return _position;
  } else {
    return 0;
  }
}

BOOL FakeFile::EndOfFile() {
  if (_isOpen) {
    return _position == _data.size();
  } else {
    return true;
  }
}

INT64 FakeFile::GetSize() { return _data.size(); }

BOOL FakeFile::IsFolder() { return _data.empty(); }

BOOL FakeFile::IsArchive() { return true; }

HRESULT FakeFile::GetLogicalName(wchar_t *path, UINT64 *length) {
  if (!path) {
    *length = _name.size();
    return S_OK;
  }
  return wmemcpy_s(path, *length, _name.data(), _name.size())
             ? E_NOT_SUFFICIENT_BUFFER
             : S_OK;
}

HRESULT FakeFile::GetPhysicalPath(wchar_t *path, UINT64 *length) {
  if (!path) {
    *length = _physicalPath.size();
    return S_OK;
  }
  return wmemcpy_s(path, *length, _physicalPath.data(), _physicalPath.size())
             ? E_NOT_SUFFICIENT_BUFFER
             : S_OK;
}

HRESULT FakeFile::GetPhysicalName(wchar_t *path, UINT64 *length) {
  return GetLogicalName(path, length);
}

}  // namespace Test
}  // namespace MGDF