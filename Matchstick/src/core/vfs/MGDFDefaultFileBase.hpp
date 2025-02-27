#pragma once

#include <MGDF/ComObject.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFStringImpl.hpp"
#include "MGDFDefaultFileReader.hpp"

namespace MGDF {
namespace core {
namespace vfs {

template <typename T>
class DefaultFileBase : public ComBase<T> {
 public:
  DefaultFileBase(const std::wstring &name,
                  const std::filesystem::path &physicalPath,
                  const std::filesystem::path &rootPath)
      : _physicalPath(physicalPath),
        _rootPath(rootPath),
        _name(name),
        _reader(nullptr) {
    std::filesystem::path relative =
        std::filesystem::proximate(_physicalPath, _rootPath);
    _logicalPath = relative.wstring();
    std::replace(_logicalPath.begin(), _logicalPath.end(), '\\', '/');
    _isFolder = is_directory(_physicalPath);
  }

  virtual ~DefaultFileBase() {}

  BOOL __stdcall IsFolder() override { return _isFolder; }

  BOOL __stdcall IsOpen() override {
    std::lock_guard<std::mutex> lock(_mutex);
    return _reader != nullptr;
  }

  HRESULT __stdcall GetPhysicalName(wchar_t *name, UINT64 *length) final {
    return StringWriter::Write(_name, name, length);
  }

  HRESULT __stdcall GetLogicalName(wchar_t *name, UINT64 *length) final {
    return StringWriter::Write(_name, name, length);
  }

  HRESULT __stdcall GetLogicalPath(wchar_t *path, UINT64 *length) final {
    return StringWriter::Write(_logicalPath, path, length);
  }

  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final {
    return StringWriter::Write(_physicalPath.wstring(), path, length);
  }

  virtual ComObject<T> CreateFile(const std::wstring &name,
                                  const std::filesystem::path &path,
                                  const std::filesystem::path &rootPath) = 0;

  BOOL GetParent(T **parent) override {
    if (_physicalPath == _rootPath) {
      *parent = nullptr;
      return false;
    }

    auto parentPath = _physicalPath.parent_path();
    auto parentName = parentPath.filename();
    auto parentFile = CreateFile(parentName.wstring(), parentPath, _rootPath);
    parentFile.AddRawRef(parent);
    return true;
  }

  UINT64 __stdcall GetChildCount() override {
    if (!_isFolder) {
      return 0U;
    }

    UINT64 count = 0;
    for (auto &p : std::filesystem::directory_iterator(_physicalPath)) {
      (void)p;
      ++count;
    }
    return count;
  }

  HRESULT __stdcall GetAllChildren(T **buffer, UINT64 *length) override {
    const UINT64 inputLength = *length;
    UINT64 count = 0;

    for (auto &p : std::filesystem::directory_iterator(_physicalPath)) {
      if (count < inputLength) {
        auto childName = p.path().filename();
        auto childFile = CreateFile(childName.wstring(), p.path(), _rootPath);
        childFile.AddRawRef(buffer++);
      }
      ++count;
    }

    *length = count;
    if (inputLength < count) {
      return E_NOT_SUFFICIENT_BUFFER;
    } else {
      return S_OK;
    }
  }

  UINT64 __stdcall GetLastWriteTime() final {
    struct _stat64 fileInfo;
    if (_wstati64(_physicalPath.c_str(), &fileInfo) != 0) {
      LOG("Unable to get last write time for "
              << Resources::ToString(_physicalPath),
          MGDF_LOG_ERROR);
      return 0;
    }
    return fileInfo.st_mtime;
  }

  HRESULT __stdcall Open(IMGDFFileReader **reader) final {
    if (IsFolder()) {
      return E_FAIL;
    }

    std::lock_guard<std::mutex> lock(_mutex);
    if (!_reader) {
      auto fileStream = std::make_shared<std::ifstream>(
          _physicalPath.c_str(),
          std::ios::in | std::ios::binary | std::ios::ate);

      if (fileStream && !fileStream->bad() && fileStream->is_open()) {
        ComObject<DefaultFileBase> self(this, true);
        _reader = new DefaultFileReader(fileStream, [self]() {
          const std::lock_guard<std::mutex> lock(self->_mutex);
          self->_reader = nullptr;
        });
        *reader = _reader;
        return S_OK;
      } else {
        LOG("Unable to open file stream for "
                << Resources::ToString(_physicalPath) << " - "
                << GetLastError(),
            MGDF_LOG_ERROR);
        return E_FAIL;
      }
    }
    LOG("File " << Resources::ToString(_physicalPath) << " currently in use",
        MGDF_LOG_ERROR);
    return E_ACCESSDENIED;
  }

  HRESULT __stdcall CopyTo(IMGDFWriteableFile *destination) final {
    if (destination->Exists()) {
      return E_FAIL;
    }

    // create all parent directories to the destination
    std::wstring destPathString =
        ComString<&IMGDFWriteableFile::GetPhysicalPath>(destination);
    std::filesystem::path destinationPath(destPathString);
    auto parentPath = destinationPath.parent_path();
    if (!exists(parentPath)) {
      std::error_code code;
      if (!create_directories(parentPath, code) || code.value()) {
        return E_FAIL;
      }
    }

    std::error_code code;
    copy(_physicalPath, destinationPath,
         IsFolder() ? std::filesystem::copy_options::recursive
                    : std::filesystem::copy_options::none,
         code);
    return !code.value() ? S_OK : E_FAIL;
  }

 protected:
  std::mutex _mutex;
  std::wstring _name;
  std::wstring _logicalPath;
  std::filesystem::path _physicalPath;
  std::filesystem::path _rootPath;
  bool _isFolder;
  DefaultFileReader *_reader;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
