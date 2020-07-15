
#include "StdAfx.h"

#include "MGDFDefaultWriteableFileImpl.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFStringImpl.hpp"

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

BOOL DefaultWriteableFileImpl::IsFolder() {
  return is_directory(_physicalPath);
}

BOOL DefaultWriteableFileImpl::IsOpen() { return _writer || _reader; }

HRESULT DefaultWriteableFileImpl::GetPhysicalName(wchar_t* name,
                                                  UINT64* length) {
  return StringWriter::Write(_name, name, length);
}

HRESULT DefaultWriteableFileImpl::GetLogicalName(wchar_t* name,
                                                 UINT64* length) {
  return StringWriter::Write(_name, name, length);
}

HRESULT DefaultWriteableFileImpl::GetPhysicalPath(wchar_t* path,
                                                  UINT64* length) {
  return StringWriter::Write(_physicalPath.wstring(), path, length);
}

BOOL DefaultWriteableFileImpl::GetParent(IMGDFWriteableFile** parent) {
  if (_physicalPath == _rootPath) {
    *parent = nullptr;
    return false;
  }

  auto parentPath = _physicalPath.parent_path();
  auto parentName = parentPath.filename();
  auto parentFile = MakeCom<DefaultWriteableFileImpl>(parentName.wstring(),
                                                      parentPath, _rootPath);
  parentFile.AddRawRef(parent);
  return true;
}

HRESULT DefaultWriteableFileImpl::GetChild(const wchar_t* name,
                                           IMGDFWriteableFile** child) {
  if (!Exists() || !IsFolder()) {
    return E_FAIL;
  }

  auto childPath = _physicalPath / name;
  auto childFile =
      MakeCom<DefaultWriteableFileImpl>(name, childPath, _rootPath);
  childFile.AddRawRef(child);
  return S_OK;
}

UINT64 DefaultWriteableFileImpl::GetChildCount() {
  if (!Exists() || !IsFolder()) {
    return 0U;
  }

  UINT64 result = 0;
  for (auto& p : directory_iterator(_physicalPath)) {
    (void)p;
    ++result;
  }
  return result;
}

HRESULT DefaultWriteableFileImpl::GetAllChildren(
    IMGDFWriteableFile** childBuffer) {
  if (!Exists() || !IsFolder()) {
    return E_FAIL;
  }

  for (auto& p : directory_iterator(_physicalPath)) {
    auto childName = p.path().filename();
    auto childFile = MakeCom<DefaultWriteableFileImpl>(childName.wstring(),
                                                       p.path(), _rootPath);
    childFile.AddRawRef(childBuffer++);
  }
  return S_OK;
}

UINT64 DefaultWriteableFileImpl::GetLastWriteTime() {
  struct _stat64 fileInfo;
  if (_wstati64(_physicalPath.c_str(), &fileInfo) != 0) {
    LOG("Unable to get last write time for "
            << Resources::ToString(_physicalPath),
        MGDF_LOG_ERROR);
    return 0;
  }
  return fileInfo.st_mtime;
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
  if (!Exists() || _physicalPath == _rootPath) {
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
  std::wstring destPathString(
      StringReader<&IMGDFWriteableFile::GetPhysicalPath>::Read(destination));
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
      _writer = new DefaultFileWriter(fileStream, [this]() {
        const std::lock_guard<std::mutex> lock(_mutex);
        _writer = nullptr;
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

HRESULT DefaultWriteableFileImpl::Open(IMGDFFileReader** reader) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (IsFolder()) {
    return E_FAIL;
  }

  if (!IsOpen()) {
    auto fileStream = std::make_shared<std::ifstream>(
        _physicalPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if (fileStream && !fileStream->bad() && fileStream->is_open()) {
      _reader = new DefaultFileReader(fileStream, [this]() {
        const std::lock_guard<std::mutex> lock(_mutex);
        _reader = nullptr;
      });
      *reader = _reader;
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
