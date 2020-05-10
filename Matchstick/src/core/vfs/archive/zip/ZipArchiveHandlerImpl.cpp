#include "StdAfx.h"

#include "ZipArchiveHandlerImpl.hpp"

#include "ZipFileRoot.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

const wchar_t *ZIP_EXT = L".zip";

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

IArchiveHandler *CreateZipArchiveHandlerImpl(IErrorHandler *errorHandler) {
  _ASSERTE(errorHandler);
  return new ZipArchiveHandlerImpl(errorHandler);
}

ZipArchiveHandlerImpl::ZipArchiveHandlerImpl(IErrorHandler *errorHandler)
    : _errorHandler(errorHandler) {
  _fileExtensions.push_back(ZIP_EXT);
}

IFile *ZipArchiveHandlerImpl::MapArchive(const wchar_t *name,
                                         const wchar_t *physicalPath,
                                         IFile *parent) {
  _ASSERTE(name);
  _ASSERTE(physicalPath);

  ZipArchive *archive = new ZipArchive(_errorHandler);
  ZipFileRoot *result = archive->MapArchive(name, physicalPath, parent);
  if (result) {
    _archives.insert(
        std::pair<ZipFileRoot *, zip::ZipArchive *>(result, archive));
  } else {
    delete archive;
  }
  return result;
}

void ZipArchiveHandlerImpl::Dispose() {
  _ASSERTE(_archives.size() == 0);
  delete this;
}

void ZipArchiveHandlerImpl::DisposeArchive(IFile *archive) {
  if (!archive) return;

  auto it = _archives.find(static_cast<ZipFileRoot *>(archive));
  _ASSERTE(it != _archives.end());
  if (it != _archives.end()) {
    delete it->first;
    delete it->second;
    _archives.erase(it);
  }
}

bool ZipArchiveHandlerImpl::IsArchive(const wchar_t *path) const {
  _ASSERTE(path);
  if (!path) return false;

  const wchar_t *extension = GetFileExtension(path);
  if (!extension) return false;

  for (auto ext : _fileExtensions) {
    if (wcscmp(ext, extension) == 0) {
      return true;
    }
  }
  return false;
}

const wchar_t *ZipArchiveHandlerImpl::GetFileExtension(
    const wchar_t *filename) const {
  _ASSERTE(filename);
  if (!filename) return nullptr;

  size_t index = wcslen(filename);
  while (index >= 0) {
    if (filename[index] == '.') return &filename[index];
    --index;
  }
  return nullptr;
}

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF