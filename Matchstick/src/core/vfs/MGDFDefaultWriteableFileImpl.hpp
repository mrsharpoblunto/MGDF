#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFStringImpl.hpp"
#include "MGDFDefaultFileReader.hpp"
#include "MGDFWriteableVirtualFileSystem.hpp"

namespace MGDF {
namespace core {
namespace vfs {

template <typename T>
class DefaultFileBase : public T {
 public:
  DefaultFileBase() : _reader(nullptr) {}
  virtual ~DefaultFileBase() {}
  BOOL __stdcall IsFolder() override { return is_directory(_physicalPath); }

  BOOL __stdcall IsOpen() override { return _reader != nullptr; }

  HRESULT __stdcall GetPhysicalName(wchar_t *name, UINT64 *length) final {
    return StringWriter::Write(_name, name, length);
  }

  HRESULT __stdcall GetLogicalName(wchar_t *name, UINT64 *length) final {
    return StringWriter::Write(_name, name, length);
  }

  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final {
    return StringWriter::Write(_physicalPath.wstring(), path, length);
  }

  UINT64 __stdcall GetChildCount() override {
    if (!IsFolder()) {
      return 0U;
    }

    UINT64 result = 0;
    for (auto &p : std::filesystem::directory_iterator(_physicalPath)) {
      (void)p;
      ++result;
    }
    return result;
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
    std::lock_guard<std::mutex> lock(_mutex);
    if (IsFolder()) {
      return E_FAIL;
    }

    if (!IsOpen()) {
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

 protected:
  std::mutex _mutex;
  std::wstring _name;
  std::filesystem::path _physicalPath;
  DefaultFileReader *_reader;
};

class DefaultFileWriter : public ComBase<IMGDFFileWriter> {
 public:
  DefaultFileWriter(std::shared_ptr<std::ofstream> stream,
                    std::function<void()> cleanup);
  virtual ~DefaultFileWriter();
  UINT32 __stdcall Write(void *buffer, UINT32 length) final;
  void __stdcall SetPosition(INT64 pos) final;
  INT64 __stdcall GetPosition() final;

 private:
  std::function<void()> _cleanup;
  std::shared_ptr<std::ofstream> _stream;
};

class DefaultWriteableFileImpl
    : public ComBase<DefaultFileBase<IMGDFWriteableFile>, IMGDFWriteableFile,
                     IMGDFFile> {
 public:
  DefaultWriteableFileImpl(const std::wstring &name,
                           const std::filesystem::path &physicalPath,
                           WriteableVirtualFileSystem *vfs)
      : _name(name),
        _physicalPath(physicalPath),
        _reader(nullptr),
        _writer(nullptr),
        _vfs(vfs) {}
  virtual ~DefaultWriteableFileImpl() {}

  BOOL __stdcall IsOpen() override;
  UINT64 __stdcall GetChildCount() override;

  BOOL __stdcall Exists() final;
  BOOL __stdcall GetParent(IMGDFWriteableFile **parent) final;
  HRESULT __stdcall GetChild(const wchar_t *name,
                             IMGDFWriteableFile **child) final;
  HRESULT __stdcall GetAllChildren(IMGDFWriteableFile **childBuffer) final;
  HRESULT __stdcall OpenWrite(IMGDFFileWriter **writer) final;
  HRESULT __stdcall GetLogicalPath(wchar_t *name, UINT64 *length) final;
  void __stdcall GetVFS(IMGDFWriteableVirtualFileSystem **vfs) final;
  HRESULT __stdcall CreateFolder() final;
  HRESULT __stdcall Delete() final;
  HRESULT __stdcall MoveTo(IMGDFWriteableFile *destination) final;
  HRESULT __stdcall CopyTo(IMGDFWriteableFile *destination) final;

 protected:
 private:
  std::mutex _mutex;
  std::wstring _name;
  std::filesystem::path _physicalPath;
  DefaultFileWriter *_writer;
  DefaultFileReader *_reader;
  WriteableVirtualFileSystem *_vfs;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
