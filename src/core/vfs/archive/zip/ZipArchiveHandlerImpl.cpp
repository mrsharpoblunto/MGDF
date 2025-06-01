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

const std::wstring S_ZIP_EXT(L".zip");
const std::wstring S_EMPTY(L"");
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
  return view.ends_with(S_ZIP_EXT);
}

BOOL ZipArchiveHandlerImpl::MapArchive(
    const wchar_t *rootPath, const wchar_t *fullPath,
    const MGDFArchivePathSegment *segments, UINT64 segmentCount,
    IMGDFArchiveHandler **handlers, UINT64 handlerCount,
    IMGDFReadOnlyFile *parent, IMGDFReadOnlyFile **file) {
  std::ignore = handlers;
  std::ignore = handlerCount;
  _ASSERTE(rootPath);
  _ASSERTE(fullPath);

  std::string fullPathUtf8 = Resources::ToString(fullPath);
  auto zip = unzOpen64(fullPathUtf8.c_str());
  if (!zip) {
    LOG("Could not open archive " << fullPathUtf8, MGDF_LOG_ERROR);
    return FALSE;
  }

  UINT64 lastModifiedTime = 0;
  struct _stat64 fileInfo;
  if (_wstati64(fullPath, &fileInfo) != 0) {
    LOG("Unable to get last write time for " << fullPathUtf8, MGDF_LOG_ERROR);
    fileInfo.st_mtime = 0;
  }
  lastModifiedTime = fileInfo.st_mtime;

  const size_t rootPathLength = wcslen(rootPath);
  const size_t fullPathLength = fullPathUtf8.size();

  std::wstring logicalPath = fullPathLength > rootPathLength
                                 ? std::wstring(fullPath + wcslen(rootPath) + 1)
                                 : S_EMPTY;
  std::replace(logicalPath.begin(), logicalPath.end(), '\\', '/');

  ComObject<ZipContext> context = MakeCom<ZipContext>();
  context->Zip = zip;
  context->LastWriteTime = lastModifiedTime;
  context->LogicalPath = logicalPath;
  context->PhysicalPath = fullPath;
  context->Parent = ComObject<IMGDFReadOnlyFile>(parent, true);

  auto rootResource =
      std::make_shared<ZipFolderImpl>(logicalPath.c_str(), nullptr, context);
  context->Resources.push_back(rootResource);

  // We need to map file positions to speed up opening later
  for (INT32 ret = unzGoToFirstFile(context->Zip); ret == UNZ_OK;
       ret = unzGoToNextFile(context->Zip)) {
    unz_file_info info;
    char nameBuffer[FILENAME_BUFFER];

    unzGetCurrentFileInfo(context->Zip, &info, nameBuffer, FILENAME_BUFFER,
                          nullptr, 0, nullptr, 0);

    // if the path is for a folder the last element will be a "" element
    // (because all path element names found using zlib include a trailing
    // "/") this means that the entire folder tree will be created in the case
    // of folders, and that the last element will be excluded for files which
    // is the desired behaviour
    const wchar_t *filename = nullptr;
    auto path = Resources::ToWString(nameBuffer);
    auto parentFolder =
        CreateParentFolder(path, context, rootResource.get(), &filename);

    if (info.uncompressed_size > 0) {
      _ASSERTE(filename);
      ZipFileHeader header;
      unzGetFilePos(context->Zip, &header.filePosition);
      header.size = info.uncompressed_size;
      header.name = filename;  // the name is the last part of the path

      auto child = std::make_shared<ZipFileImpl>(filename, parentFolder,
                                                 context, std::move(header));
      parentFolder->AddChild(child);
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
    std::wstring &path, ComObject<ZipContext> &context, ZipFolderImpl *root,
    const wchar_t **filename) {
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
  ZipFolderImpl *parent = root;

  while (parent && end < len) {
    while (end < len && path[end] != '/') {
      ++end;
    }
    if (end != start) {
      path[end] = '\0';
      ComObject<IMGDFReadOnlyFile> child;
      wchar_t *name = &path[start];
      if (!parent->GetChild(name, child.Assign())) {
        auto newChild = std::make_shared<ZipFolderImpl>(name, parent, context);
        child = ComObject<IMGDFReadOnlyFile>(newChild.get(), true);
        parent->AddChild(newChild);
      }
      parent = dynamic_cast<ZipFolderImpl *>(child.Get());
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