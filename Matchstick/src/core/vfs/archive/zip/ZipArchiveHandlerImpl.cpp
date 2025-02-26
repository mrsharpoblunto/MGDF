#include "StdAfx.h"

#include "ZipArchiveHandlerImpl.hpp"

#include <algorithm>

#include "../../../common/MGDFLoggerImpl.hpp"
#include "../../../common/MGDFResources.hpp"
#include "ZipFileImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

const std::wstring ZIP_EXT(L".zip");
#define FILENAME_BUFFER 512

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

ComObject<IMGDFArchiveHandler> CreateZipArchiveHandlerImpl() {
  return MakeCom<ZipArchiveHandlerImpl>().As<IMGDFArchiveHandler>();
}

ZipArchiveHandlerImpl::ZipArchiveHandlerImpl() {}

ZipArchiveHandlerImpl::~ZipArchiveHandlerImpl() {}

BOOL ZipArchiveHandlerImpl::TestPathSegment(
    const MGDFArchivePathSegment *segment) {
  const std::wstring_view view(segment->Start, segment->Length);
  return view.ends_with(ZIP_EXT);
}

BOOL ZipArchiveHandlerImpl::MapArchive(const wchar_t *rootPath,
                                       const wchar_t *fullPath,
                                       const MGDFArchivePathSegment *segments,
                                       UINT64 segmentCount,
                                       IMGDFReadOnlyFile **file) {
  _ASSERTE(rootPath);
  _ASSERTE(fullPath);

  auto zip = unzOpen64(Resources::ToString(fullPath).c_str());
  if (!zip) {
    LOG("Could not open archive " << Resources::ToString(fullPath),
        MGDF_LOG_ERROR);
    return FALSE;
  }

  UINT64 lastModifiedTime = 0;
  struct _stat64 fileInfo;
  if (_wstati64(fullPath, &fileInfo) != 0) {
    LOG("Unable to get last write time for " << Resources::ToString(fullPath),
        MGDF_LOG_ERROR);
    fileInfo.st_mtime = 0;
  }
  lastModifiedTime = fileInfo.st_mtime;

  std::wstring logicalPath(fullPath + wcslen(rootPath));
  std::replace(logicalPath.begin(), logicalPath.end(), '\\', '/');

  auto archive =
      MakeCom<ZipArchive>(logicalPath, fullPath, lastModifiedTime, zip);
  auto rootResource =
      std::make_shared<ZipFolderImpl>(logicalPath.c_str(), nullptr, archive);
  archive->AddResource(rootResource);

  // We need to map file positions to speed up opening later
  for (INT32 ret = unzGoToFirstFile(archive->GetZip()); ret == UNZ_OK;
       ret = unzGoToNextFile(archive->GetZip())) {
    unz_file_info info;
    char nameBuffer[FILENAME_BUFFER];

    unzGetCurrentFileInfo(archive->GetZip(), &info, nameBuffer, FILENAME_BUFFER,
                          nullptr, 0, nullptr, 0);

    // if the path is for a folder the last element will be a "" element
    // (because all path element names found using zlib include a trailing
    // "/") this means that the entire folder tree will be created in the case
    // of folders, and that the last element will be excluded for files which
    // is the desired behaviour
    const wchar_t *filename = nullptr;
    auto path = Resources::ToWString(nameBuffer);
    auto parent =
        CreateParentFolder(path, archive.Get(), rootResource.get(), &filename);

    if (info.uncompressed_size > 0) {
      _ASSERTE(filename);
      ZipFileHeader header;
      unzGetFilePos(archive->GetZip(), &header.filePosition);
      header.size = info.uncompressed_size;
      header.name = filename;  // the name is the last part of the path

      auto child = std::make_shared<ZipFileImpl>(filename, parent, archive,
                                                 std::move(header));
      parent->AddChild(child);
    }
  }

  // if the logical path was a file within the archive, we need to traverse
  // the components to get that, rather than just returning the root of the archive
  ComObject<IMGDFReadOnlyFile> current(rootResource.get(), true);
  for (UINT64 i = 0; i < segmentCount; ++i) {
    std::wstring childName(segments[i].Start, segments[i].Length);
    ComObject<IMGDFReadOnlyFile> child;
    if (current->GetChild(childName.c_str(), child.Assign())) {
      current = child;
    } else {
      return FALSE;
    }
  }
  current.AddRawRef(file);
  return TRUE;
}

ZipFolderImpl *ZipArchiveHandlerImpl::CreateParentFolder(
    std::wstring &path, ZipArchive *archive, ZipFolderImpl *root,
    const wchar_t **filename) {
  _ASSERTE(root);
  _ASSERTE(archive);
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
  ZipFolderImpl *parent = root;

  while (end < len) {
    while (end < len && path[end] != '/') {
      ++end;
    }
    if (end != start) {
      path[end] = '\0';
      ComObject<IMGDFReadOnlyFile> child;
      wchar_t *name = &path[start];
      if (!parent->GetChild(name, child.Assign())) {
        auto newChild = std::make_shared<ZipFolderImpl>(name, parent, archive);
        child = ComObject<IMGDFReadOnlyFile>(newChild.get(), true);
        parent->AddChild(newChild);
      }
      parent = static_cast<ZipFolderImpl *>(child.Get());
    }
    ++end;
    start = end;
  }

  return parent;
}

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF