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

ComObject<IVirtualFileSystemComponent> CreateVirtualFileSystemComponentImpl() {
  return ComObject<IVirtualFileSystemComponent>(
      new VirtualFileSystemComponent());
}

VirtualFileSystemComponent::VirtualFileSystemComponent() {}

bool VirtualFileSystemComponent::Mount(const wchar_t *physicalDirectory) {
  _ASSERTE(physicalDirectory);
  _ASSERTE(!_root);
  Map(physicalDirectory, ComObject<IFile>(), _root);
  return _root && _root->IsFolder();
}

void VirtualFileSystemComponent::Map(const path &path, ComObject<IFile> parent,
                                     ComObject<IFile> &child) {
  // wpath path( physicalPath );
  if (is_directory(path)) {
    child =
        new DefaultFolderImpl(path.filename(), path.wstring(), parent, this);
  } else {
    // if its an archive
    ComObject<IArchiveHandler> archiveHandler;
    if (GetArchiveHandler(path.wstring(), archiveHandler)) {
      auto filename = path.filename();
      auto fullpath = path.wstring();
      ComObject<IFile> mappedFile;
      if (!FAILED(archiveHandler->MapArchive(filename.c_str(), fullpath.c_str(),
                                             parent, mappedFile.Assign()))) {
        child = mappedFile;
        return;
      } else {
        LOG("Unable to map archive " << Resources::ToString(path.wstring()),
            LOG_ERROR);
      }
    }

    // otherwise its just a plain old file
    child = new DefaultFileImpl(path.filename(), path.wstring(), parent);
  }
}

bool VirtualFileSystemComponent::GetArchiveHandler(
    const std::wstring &path, ComObject<IArchiveHandler> &handler) {
  for (auto h : _archiveHandlers) {
    if (h->IsArchive(path.c_str())) {
      handler = h;
      return true;
    }
  }
  return false;
}

bool VirtualFileSystemComponent::GetFile(const wchar_t *logicalPath,
                                         IFile **file) {
  if (!logicalPath) {
    _root.AddRawRef(file);
    return true;
  }

  wchar_t *context = 0;
  const size_t destinationLength = wcslen(logicalPath) + 1;
  std::vector<wchar_t> copy(destinationLength);
  wcscpy_s(copy.data(), destinationLength, logicalPath);
  wchar_t *components = wcstok_s(copy.data(), L"/", &context);

  ComObject<IFile> node(_root);
  while (components) {
    ComObject<IFile> tmp;
    if (!node->GetChild(components, tmp.Assign())) {
      return false;
    }
    node = tmp;
    components = wcstok_s(0, L"/", &context);
  }

  node.AddRawRef(file);
  return true;
}

void VirtualFileSystemComponent::GetRoot(IFile **root) {
  _root.AddRawRef(root);
}

void VirtualFileSystemComponent::RegisterArchiveHandler(
    ComObject<IArchiveHandler> handler) {
  _ASSERTE(handler);
  _archiveHandlers.push_back(handler);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF