#include "stdafx.h"

#include "MGDFWriteableVirtualFileSystem.hpp"

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

DefaultWriteableFileImpl::DefaultWriteableFileImpl(
    const std::wstring& name, const std::filesystem::path& physicalPath,
    std::shared_ptr<DefaultFileBaseContext> context)
    : DefaultFileBase(name, physicalPath, context), _writer(nullptr) {}

ComObject<IMGDFWriteableFile> DefaultWriteableFileImpl::CreateFile(
    const std::wstring& name, const std::filesystem::path& path) {
  return MakeCom<DefaultWriteableFileImpl>(name, path, _context)
      .As<IMGDFWriteableFile>();
}

BOOL DefaultWriteableFileImpl::Exists() { return exists(_physicalPath); }

UINT64 DefaultWriteableFileImpl::GetChildCount() {
  if (!Exists()) {
    return 0U;
  } else {
    return DefaultFileBase::GetChildCount();
  }
}

HRESULT DefaultWriteableFileImpl::GetAllChildren(
    IMGDFWriteableFile** childBuffer, UINT64* length) {
  if (!Exists()) {
    *length = 0;
    return S_OK;
  }
  return DefaultFileBase::GetAllChildren(childBuffer, length);
}

BOOL DefaultWriteableFileImpl::GetChild(const wchar_t* name,
                                        IMGDFWriteableFile** child) {
  if (!Exists()) {
    return FALSE;
  } else {
    const auto childPath = _physicalPath / name;
    auto childFile =
        MakeCom<DefaultWriteableFileImpl>(name, childPath, _context);
    childFile.AddRawRef(child);
    return TRUE;
  }
}

HRESULT DefaultWriteableFileImpl::CreateFolder() {
  if (Exists()) {
    return E_FAIL;
  }

  std::error_code code;
  const auto result = create_directories(_physicalPath, code) && !code.value();
  if (result) {
    _isFolder = true;
  }

  return result ? S_OK : E_FAIL;
}

HRESULT DefaultWriteableFileImpl::Delete() {
  if (!Exists() || _physicalPath == _context->RootPath) {
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

BOOL DefaultWriteableFileImpl::IsOpen() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _writer || _reader;
}

HRESULT DefaultWriteableFileImpl::OpenWrite(IMGDFFileWriter** writer) {
  if (IsFolder()) {
    return E_FAIL;
  }

  std::lock_guard<std::mutex> lock(_mutex);
  if (!_writer && !_reader) {
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

WriteableVirtualFileSystem::WriteableVirtualFileSystem(
    const std::wstring& rootPath)
    : _context(std::make_shared<DefaultFileBaseContext>()) {
  _context->RootPath = std::filesystem::path(rootPath).lexically_normal();
  if (!_context->RootPath.has_filename()) {
    _context->RootPath = _context->RootPath.parent_path();
  }
}

BOOL WriteableVirtualFileSystem::GetFile(const wchar_t* logicalPath,
                                         IMGDFWriteableFile** file) {
  if (!logicalPath) {
    GetRoot(file);
    return true;
  }

  std::wstring_view logicalPathView(logicalPath);
  if (logicalPathView.at(0) == L'/') {
    // If the logical path starts with a '/', we assume it's root-relative.
    // We can remove the leading '/' to make it relative to the root path.
    logicalPathView.remove_prefix(1);
  }

  std::filesystem::path path =
      (_context->RootPath / logicalPathView).lexically_normal();

  auto node = MakeCom<DefaultWriteableFileImpl>(path.filename().c_str(), path,
                                                _context);
  node.AddRawRef(file);
  return true;
}

void WriteableVirtualFileSystem::GetRoot(IMGDFWriteableFile** root) {
  auto r = MakeCom<DefaultWriteableFileImpl>(
      _context->RootPath.filename().c_str(), _context->RootPath, _context);
  r.AddRawRef(root);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
