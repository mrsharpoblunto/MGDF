#include "stdafx.h"

#include "MGDFReadOnlyVirtualFileSystemComponentImpl.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFDefaultReadOnlyFileImpl.hpp"
#include "MGDFDefaultReadOnlyFolderImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

bool CreateReadOnlyVirtualFileSystemComponentImpl(
    ComObject<IReadOnlyVirtualFileSystemComponent> &comp) {
  comp = ComObject<IReadOnlyVirtualFileSystemComponent>(
      new ReadOnlyVirtualFileSystemComponent());
  return true;
}

ReadOnlyVirtualFileSystemComponent::ReadOnlyVirtualFileSystemComponent() {}

bool ReadOnlyVirtualFileSystemComponent::Mount(
    const wchar_t *physicalDirectory) {
  _ASSERTE(physicalDirectory);
  _ASSERTE(!_root);
  Map(physicalDirectory, ComObject<IMGDFReadOnlyFile>(), _root);
  return _root && _root->IsFolder();
}

void ReadOnlyVirtualFileSystemComponent::Map(
    const path &path, ComObject<IMGDFReadOnlyFile> parent,
    ComObject<IMGDFReadOnlyFile> &child) {
  // wpath path( physicalPath );
  if (is_directory(path)) {
    child = ComObject<IMGDFReadOnlyFile>(new DefaultReadOnlyFolderImpl(
        path.filename(), path.wstring(), parent, this));
  } else {
    // if its an archive
    ComObject<IMGDFArchiveHandler> archiveHandler;
    if (GetArchiveHandler(path.wstring(), archiveHandler)) {
      auto filename = path.filename();
      auto fullpath = path.wstring();
      ComObject<IMGDFReadOnlyFile> mappedFile;
      if (!FAILED(archiveHandler->MapArchive(filename.c_str(), fullpath.c_str(),
                                             parent, mappedFile.Assign()))) {
        child = mappedFile;
        return;
      } else {
        LOG("Unable to map archive " << Resources::ToString(path.wstring()),
            MGDF_LOG_ERROR);
      }
    }

    // otherwise its just a plain old file
    child = ComObject<IMGDFReadOnlyFile>(
        new DefaultReadOnlyFileImpl(path.filename(), path.wstring(), parent));
  }
}

bool ReadOnlyVirtualFileSystemComponent::GetArchiveHandler(
    const std::wstring &path, ComObject<IMGDFArchiveHandler> &handler) {
  for (auto h : _archiveHandlers) {
    if (h->IsArchive(path.c_str())) {
      handler = h;
      return true;
    }
  }
  return false;
}

BOOL ReadOnlyVirtualFileSystemComponent::GetFile(const wchar_t *logicalPath,
                                                 IMGDFReadOnlyFile **file) {
  if (!logicalPath) {
    _root.AddRawRef(file);
    return true;
  }

  wchar_t *context = 0;
  const size_t destinationLength = wcslen(logicalPath) + 1;
  std::vector<wchar_t> copy(destinationLength);
  wcscpy_s(copy.data(), destinationLength, logicalPath);
  wchar_t *components = wcstok_s(copy.data(), L"/", &context);

  ComObject<IMGDFReadOnlyFile> node(_root);
  while (components) {
    ComObject<IMGDFReadOnlyFile> tmp;
    if (!node->GetChild(components, tmp.Assign())) {
      return false;
    }
    node = tmp;
    components = wcstok_s(0, L"/", &context);
  }

  node.AddRawRef(file);
  return true;
}

void ReadOnlyVirtualFileSystemComponent::GetRoot(IMGDFReadOnlyFile **root) {
  _root.AddRawRef(root);
}

void ReadOnlyVirtualFileSystemComponent::RegisterArchiveHandler(
    ComObject<IMGDFArchiveHandler> handler) {
  _ASSERTE(handler);
  _archiveHandlers.push_back(handler);
}

HRESULT __stdcall ReadOnlyVirtualFileSystemComponent::GetLogicalPath(
    IMGDFReadOnlyFile *file, wchar_t *path, UINT64 *length) {
  return _resolver.GetLogicalPath(file, path, length);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF