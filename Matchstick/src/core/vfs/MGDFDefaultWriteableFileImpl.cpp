
#include "StdAfx.h"

#include "MGDFDefaultWriteableFileImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

DefaultFileWriter::DefaultFileWriter(std::shared_ptr<std::ofstream> stream,
                                     std::function<void()> cleanup)
    : _stream(stream), _cleanup(cleanup) {}

DefaultFileWriter::~DefaultFileWriter() {
  _stream->close();
  _cleanup();
}

UINT32 DefaultFileWriter::Write(void* buffer, UINT32 length) {
  if (buffer && length) {
    const auto oldPosition = _stream->tellp();
    _stream->write(static_cast<char*>(buffer), length);
    const auto newPosition = _stream->tellp();
    return static_cast<UINT32>(newPosition - oldPosition);
  }
  return 0;
}

void DefaultFileWriter::SetPosition(INT64 pos) { _stream->seekp(pos); }

INT64 DefaultFileWriter::GetPosition() { return _stream->tellp(); }

BOOL DefaultWriteableFileImpl::Exists() { return exists(_physicalPath); }

void DefaultWriteableFileImpl::GetVFS(IMGDFWriteableVirtualFileSystem** vfs) {
  _vfs->AddRef();
  *vfs = _vfs;
}

HRESULT DefaultWriteableFileImpl::GetLogicalPath(wchar_t* path,
                                                 UINT64* length) {
  return _vfs->GetLogicalPath(this, path, length);
}

BOOL DefaultWriteableFileImpl::GetParent(IMGDFWriteableFile** parent) {
  if (_physicalPath == _vfs->GetRootPath()) {
    *parent = nullptr;
    return false;
  }

  auto parentPath = _physicalPath.parent_path();
  auto parentName = parentPath.filename();
  auto parentFile =
      MakeCom<DefaultWriteableFileImpl>(parentName.wstring(), parentPath, _vfs);
  parentFile.AddRawRef(parent);
  return true;
}

HRESULT DefaultWriteableFileImpl::GetChild(const wchar_t* name,
                                           IMGDFWriteableFile** child) {
  if (!Exists() || !IsFolder()) {
    return E_FAIL;
  }

  auto childPath = _physicalPath / name;
  auto childFile = MakeCom<DefaultWriteableFileImpl>(name, childPath, _vfs);
  childFile.AddRawRef(child);
  return S_OK;
}

UINT64 DefaultWriteableFileImpl::GetChildCount() {
  if (!Exists()) {
    return 0U;
  } else {
    return DefaultFileBase::GetChildCount();
  }
}

HRESULT DefaultWriteableFileImpl::GetAllChildren(
    IMGDFWriteableFile** childBuffer) {
  if (!Exists() || !IsFolder()) {
    return E_FAIL;
  }

  for (auto& p : directory_iterator(_physicalPath)) {
    auto childName = p.path().filename();
    auto childFile =
        MakeCom<DefaultWriteableFileImpl>(childName.wstring(), p.path(), _vfs);
    childFile.AddRawRef(childBuffer++);
  }
  return S_OK;
}

HRESULT DefaultWriteableFileImpl::CreateFolder() {
  if (Exists()) {
    return E_FAIL;
  }

  std::error_code code;
  return (create_directories(_physicalPath, code) && !code.value()) ? S_OK
                                                                    : E_FAIL;
}

HRESULT DefaultWriteableFileImpl::Delete() {
  if (!Exists() || _physicalPath == _vfs->GetRootPath()) {
    return E_FAIL;
  }
  std::error_code code;
  return (remove_all(_physicalPath, code) && !code.value()) ? S_OK : E_FAIL;
}

HRESULT DefaultWriteableFileImpl::MoveTo(IMGDFWriteableFile* destination) {
  if (!Exists() || destination->Exists()) {
    return E_FAIL;
  }

  // create all parent directories to the destination
  std::wstring destPathString =
      ComString<&IMGDFWriteableFile::GetPhysicalPath>(destination);
  path destinationPath(destPathString);
  auto parentPath = destinationPath.parent_path();
  if (!exists(parentPath)) {
    std::error_code code;
    if (!create_directories(parentPath, code) || code.value()) {
      return E_FAIL;
    }
  }

  std::error_code code;
  rename(_physicalPath, destinationPath, code);
  return !code.value() ? S_OK : E_FAIL;
}

HRESULT DefaultWriteableFileImpl::CopyTo(IMGDFWriteableFile* destination) {
  if (destination->Exists()) {
    return E_FAIL;
  }

  // create all parent directories to the destination
  std::wstring destPathString =
      ComString<&IMGDFWriteableFile::GetPhysicalPath>(destination);
  path destinationPath(destPathString);
  auto parentPath = destinationPath.parent_path();
  if (!exists(parentPath)) {
    std::error_code code;
    if (!create_directories(parentPath, code) || code.value()) {
      return E_FAIL;
    }
  }

  std::error_code code;
  copy(_physicalPath, destinationPath,
       IsFolder() ? copy_options::recursive : copy_options::none, code);
  return !code.value() ? S_OK : E_FAIL;
}

BOOL DefaultWriteableFileImpl::IsOpen() {
  return _writer || DefaultFileBase::IsOpen();
}

HRESULT DefaultWriteableFileImpl::OpenWrite(IMGDFFileWriter** writer) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (IsFolder()) {
    return E_FAIL;
  }

  if (!IsOpen()) {
    if (!Exists()) {
      // create all parent directories
      auto parentPath = _physicalPath.parent_path();
      if (!exists(parentPath)) {
        std::error_code code;
        if (!create_directories(parentPath, code) || code.value()) {
          return E_FAIL;
        }
      }
    }

    auto fileStream = std::make_shared<std::ofstream>(
        _physicalPath.c_str(), std::ios::out | std::ios::binary);

    if (fileStream && !fileStream->bad() && fileStream->is_open()) {
      ComObject<DefaultWriteableFileImpl> self(this, true);
      _writer = new DefaultFileWriter(fileStream, [self]() {
        const std::lock_guard<std::mutex> lock(self->_mutex);
        self->_writer = nullptr;
      });
      *writer = _writer;
      return S_OK;
    } else {
      LOG("Unable to open file stream for "
              << Resources::ToString(_physicalPath) << " - " << GetLastError(),
          MGDF_LOG_ERROR);
      return E_FAIL;
    }
  }
  LOG("File " << Resources::ToString(_physicalPath) << " currently in use",
      MGDF_LOG_ERROR);
  return E_ACCESSDENIED;
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
