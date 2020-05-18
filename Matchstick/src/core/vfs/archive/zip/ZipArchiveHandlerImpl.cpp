#include "StdAfx.h"

#include "ZipArchiveHandlerImpl.hpp"

#include "../../../common/MGDFLoggerImpl.hpp"
#include "../../../common/MGDFResources.hpp"
#include "ZipFileImpl.hpp"
#include "ZipFileRoot.hpp"
#include "ZipFolderImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

const wchar_t *ZIP_EXT = L".zip";
#define FILENAME_BUFFER 512

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

ComObject<IArchiveHandler> CreateZipArchiveHandlerImpl() {
  return ComObject<IArchiveHandler>(new ZipArchiveHandlerImpl());
}

ZipArchiveHandlerImpl::ZipArchiveHandlerImpl() {
  _fileExtensions.push_back(ZIP_EXT);
}

ZipArchiveHandlerImpl::~ZipArchiveHandlerImpl() {}

HRESULT ZipArchiveHandlerImpl::MapArchive(const wchar_t *name,
                                          const wchar_t *physicalPath,
                                          IFile *parent, IFile **file) {
  _ASSERTE(name);
  _ASSERTE(physicalPath);

  auto zip = unzOpen(physicalPath);

  if (zip) {
    ComObject<IFile> root(new ZipFileRoot(name, physicalPath, parent, zip));

    // We need to map file positions to speed up opening later
    for (INT32 ret = unzGoToFirstFile(zip); ret == UNZ_OK;
         ret = unzGoToNextFile(zip)) {
      unz_file_info info;
      char name[FILENAME_BUFFER];

      unzGetCurrentFileInfo(zip, &info, name, FILENAME_BUFFER, nullptr, 0,
                            nullptr, 0);

      // if the path is for a folder the last element will be a "" element
      // (because all path element names found using zlib include a trailing
      // "/") this means that the entire folder tree will be created in the case
      // of folders, and that the last element will be excluded for files which
      // is the desired behaviour
      const wchar_t *filename = nullptr;
      std::wstring path = Resources::ToWString(name);
      ComObject<IFile> parentFile = CreateParentFile(path, root, &filename);

      if (info.uncompressed_size > 0) {
        _ASSERTE(filename);
        ZipFileHeader header;
        unzGetFilePos(zip, &header.filePosition);
        header.size = info.uncompressed_size;
        header.name = filename;  // the name is the last part of the path

        static_cast<FileBaseImpl *>(parentFile.Get())
            ->AddChild(ComObject<IFile>(
                new ZipFileImpl(parentFile, root, zip, std::move(header))));
      }
    }
    root.AddRawRef(file);
    return S_OK;
  } else {
    LOG("Could not open archive " << Resources::ToString(physicalPath),
        LOG_ERROR);
    return ERROR_OPEN_FAILED;
  }
}

ComObject<IFile> ZipArchiveHandlerImpl::CreateParentFile(
    std::wstring &path, ComObject<IFile> root, const wchar_t **filename) {
  _ASSERTE(root);
  _ASSERTE(path.size());

  size_t len = path.rfind('/');
  if (len == std::wstring::npos) {
    *filename = path.data();
    len = 0;
  } else {
    path[len] = '\0';
    *filename = &path.data()[len + 1];
  }

  size_t start = 0;
  size_t end = 0;
  ComObject<IFile> parent(root);

  while (end < len) {
    while (end < len && path[end] != '/') {
      ++end;
    }
    if (end != start) {
      path[end] = '\0';
      ComObject<IFile> child;
      if (!parent->GetChild(&path[start], child.Assign())) {
        child = new ZipFolderImpl(&path[start], parent, root);
        static_cast<FileBaseImpl *>(parent.Get())->AddChild(child);
      }
      parent = child;
    }
    ++end;
    start = end;
  }

  return parent;
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