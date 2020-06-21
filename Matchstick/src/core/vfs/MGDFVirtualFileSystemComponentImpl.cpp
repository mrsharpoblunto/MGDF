#include "stdafx.h"

#include "MGDFVirtualFileSystemComponentImpl.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFDefaultFileImpl.hpp"
#include "MGDFDefaultFolderImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

bool CreateVirtualFileSystemComponentImpl(
    ComObject<IVirtualFileSystemComponent> &comp) {
  comp =
      ComObject<IVirtualFileSystemComponent>(new VirtualFileSystemComponent());
  return true;
}

VirtualFileSystemComponent::VirtualFileSystemComponent() {}

bool VirtualFileSystemComponent::Mount(const wchar_t *physicalDirectory) {
  _ASSERTE(physicalDirectory);
  _ASSERTE(!_root);
  Map(physicalDirectory, ComObject<IMGDFFile>(), _root);
  return _root && _root->IsFolder();
}

void VirtualFileSystemComponent::Map(const path &path, ComObject<IMGDFFile> parent,
                                     ComObject<IMGDFFile> &child) {
  // wpath path( physicalPath );
  if (is_directory(path)) {
    child = ComObject<IMGDFFile>(
        new DefaultFolderImpl(path.filename(), path.wstring(), parent, this));
  } else {
    // if its an archive
    ComObject<IMGDFArchiveHandler> archiveHandler;
    if (GetArchiveHandler(path.wstring(), archiveHandler)) {
      auto filename = path.filename();
      auto fullpath = path.wstring();
      ComObject<IMGDFFile> mappedFile;
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
    child = ComObject<IMGDFFile>(
        new DefaultFileImpl(path.filename(), path.wstring(), parent));
  }
}

bool VirtualFileSystemComponent::GetArchiveHandler(
    const std::wstring &path, ComObject<IMGDFArchiveHandler> &handler) {
  for (auto h : _archiveHandlers) {
    if (h->IsArchive(path.c_str())) {
      handler = h;
      return true;
    }
  }
  return false;
}

BOOL VirtualFileSystemComponent::GetFile(const wchar_t *logicalPath,
                                         IMGDFFile **file) {
  if (!logicalPath) {
    _root.AddRawRef(file);
    return true;
  }

  wchar_t *context = 0;
  const size_t destinationLength = wcslen(logicalPath) + 1;
  std::vector<wchar_t> copy(destinationLength);
  wcscpy_s(copy.data(), destinationLength, logicalPath);
  wchar_t *components = wcstok_s(copy.data(), L"/", &context);

  ComObject<IMGDFFile> node(_root);
  while (components) {
    ComObject<IMGDFFile> tmp;
    if (!node->GetChild(components, tmp.Assign())) {
      return false;
    }
    node = tmp;
    components = wcstok_s(0, L"/", &context);
  }

  node.AddRawRef(file);
  return true;
}

void VirtualFileSystemComponent::GetRoot(IMGDFFile **root) {
  _root.AddRawRef(root);
}

void VirtualFileSystemComponent::RegisterArchiveHandler(
    ComObject<IMGDFArchiveHandler> handler) {
  _ASSERTE(handler);
  _archiveHandlers.push_back(handler);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF