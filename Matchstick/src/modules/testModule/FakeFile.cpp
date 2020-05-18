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
                   MGDF::IFile *parent)
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
  ULONG refs = --_references;
  if (refs == 0UL) {
    delete this;
  };
  return refs;
}

HRESULT FakeFile::QueryInterface(REFIID riid, void **ppvObject) {
  if (!ppvObject) return E_POINTER;
  if (riid == IID_IUnknown || riid == __uuidof(MGDF::IFile) ||
      (_isOpen && riid == _uuidof(MGDF::IFileReader))) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  }
  return E_NOINTERFACE;
};

bool FakeFile::GetParent(IFile **parent) {
  _parent->AddRef();
  *parent = _parent;
  return true;
}

size_t FakeFile::GetChildCount() {
  if (_children) {
    return _children->size();
  }
  return 0;
}

time_t FakeFile::GetLastWriteTime() const { return 0; }

bool FakeFile::GetChild(const wchar_t *name, IFile **child) {
  if (!_children || !name) return false;

  auto it = _children->find(name);
  if (it != _children->end()) {
    it->second.AddRawRef(child);
    return true;
  }
  return false;
}

void FakeFile::GetAllChildren(IFile **childBuffer) {
  for (auto child : *_children) {
    child.second.AddRawRef(childBuffer++);
  }
}

void FakeFile::AddChild(ComObject<FakeFile> file) {
  _ASSERTE(file);
  if (!_children) {
    _children = std::make_unique<
        std::map<const wchar_t *, ComObject<FakeFile>, WCharCmp>>();
  }
  _children->insert(std::make_pair(file->GetName(), std::move(file)));
}

const wchar_t *FakeFile::GetLogicalPath() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_logicalPath.empty()) {
    std::vector<ComObject<IFile>> path;
    ComObject<IFile> node(this, true);
    do {
      path.push_back(node);
    } while (node->GetParent(node.Assign()));

    std::wostringstream ss;
    for (auto file : path) {
      ss << file->GetName();
      if (file != this) ss << '/';
    }
    _logicalPath = ss.str();
  }

  return _logicalPath.c_str();
}

bool FakeFile::IsOpen() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _isOpen;
}

HRESULT FakeFile::Open(IFileReader **reader) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_data.empty() && !_isOpen) {
    _isOpen = true;
    _position = 0;
    this->AddRef();
    *reader = this;
    return S_OK;
  }
  return ERROR_ACCESS_DENIED;
}

UINT32 FakeFile::Read(void *buffer, UINT32 length) {
  if (_isOpen) {
    INT32 oldPosition = _position;
    if ((static_cast<UINT32>(oldPosition) + length) > _data.size())
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

INT64 FakeFile::GetPosition() const {
  if (_isOpen) {
    return _position;
  } else {
    return 0;
  }
}

bool FakeFile::EndOfFile() const {
  if (_isOpen) {
    return _position == _data.size();
  } else {
    return true;
  }
}

INT64 FakeFile::GetSize() const { return _data.size(); }

bool FakeFile::IsFolder() const { return _data.empty(); }

bool FakeFile::IsArchive() const { return true; }

const wchar_t *FakeFile::GetArchiveName() const {
  return _physicalPath.c_str();
}

const wchar_t *FakeFile::GetPhysicalPath() const {
  return _physicalPath.c_str();
}

const wchar_t *FakeFile::GetName() const { return _name.c_str(); }

}  // namespace Test
}  // namespace MGDF