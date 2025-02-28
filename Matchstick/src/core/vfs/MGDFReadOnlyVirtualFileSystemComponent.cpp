#include "stdafx.h"

#include "MGDFReadOnlyVirtualFileSystemComponent.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

DefaultReadOnlyFileImpl::DefaultReadOnlyFileImpl(
    const std::wstring &name, const std::filesystem::path &physicalPath,
    const std::filesystem::path &rootPath, IMGDFArchiveHandler **handlers,
    size_t handlerCount)
    : DefaultFileBase(name, physicalPath, rootPath),
      _handlers(handlers),
      _handlerCount(handlerCount) {
  for (size_t i = 0; i < handlerCount; ++i) {
    handlers[i]->AddRef();
  }
}

DefaultReadOnlyFileImpl::~DefaultReadOnlyFileImpl() {
  for (size_t i = 0; i < _handlerCount; ++i) {
    _handlers[i]->Release();
  }
}

ComObject<IMGDFReadOnlyFile> DefaultReadOnlyFileImpl::CreateFile(
    const std::wstring &name, const std::filesystem::path &path,
    const std::filesystem::path &rootPath) {
  return MakeCom<DefaultReadOnlyFileImpl>(name, path, rootPath, _handlers,
                                          _handlerCount)
      .As<IMGDFReadOnlyFile>();
}

BOOL DefaultReadOnlyFileImpl::GetChild(const wchar_t *name,
                                       IMGDFReadOnlyFile **child) {
  const auto childPath = _physicalPath / name;

  if (_handlerCount) {
    const std::wstring_view view(name);
    const MGDFArchivePathSegment segment{.Start = name, .Length = view.size()};
    for (size_t i = 0; i < _handlerCount; ++i) {
      if (_handlers[i]->TestPathSegment(&segment)) {
        if (_handlers[i]->MapArchive(_rootPath.c_str(),
                                     childPath.wstring().c_str(), nullptr, 0,
                                     _handlers, _handlerCount, this, child)) {
          return TRUE;
        }
      }
    }
  }

  if (!std::filesystem::exists(childPath)) {
    return FALSE;
  }

  auto childFile = MakeCom<DefaultReadOnlyFileImpl>(name, childPath, _rootPath,
                                                    _handlers, _handlerCount);
  childFile.AddRawRef(child);
  return TRUE;
}

ReadOnlyVirtualFileSystemComponent::ReadOnlyVirtualFileSystemComponent()
    : _rootParentPathSize(0) {}

ReadOnlyVirtualFileSystemComponent::~ReadOnlyVirtualFileSystemComponent() {
  for (auto handler : _archiveHandlers) {
    handler->Release();
  }
}

bool ReadOnlyVirtualFileSystemComponent::Mount(
    const wchar_t *physicalDirectory) {
  _rootPath = std::filesystem::path(physicalDirectory).lexically_normal();
  if (!_rootPath.has_filename()) {
    _rootPath = _rootPath.parent_path();
  }
  _rootParentPathSize = _rootPath.parent_path().wstring().size();
  return std::filesystem::is_directory(_rootPath);
}

BOOL ReadOnlyVirtualFileSystemComponent::GetFile(const wchar_t *logicalPath,
                                                 IMGDFReadOnlyFile **file) {
  std::filesystem::path path = (_rootPath / logicalPath).lexically_normal();
  return Get(path, file);
}

void ReadOnlyVirtualFileSystemComponent::GetRoot(IMGDFReadOnlyFile **root) {
  Get(_rootPath, root);
}

BOOL ReadOnlyVirtualFileSystemComponent::Get(const std::filesystem::path &path,
                                             IMGDFReadOnlyFile **file) {
  if (!_archiveHandlers.empty()) {
    std::wstring pathStr = path.wstring();
    std::vector<MGDFArchivePathSegment> segments;

    // Split path into segments
    size_t start = _rootParentPathSize + 1;
    size_t pos = 0;
    while ((pos = pathStr.find_first_of(L"/\\", start)) != std::wstring::npos) {
      MGDFArchivePathSegment &segment = segments.emplace_back();
      segment.Start = pathStr.c_str() + start;
      segment.Length = pos - start;
      start = pos + 1;
    }

    // Add the last segment if it exists
    if (start < pathStr.length()) {
      MGDFArchivePathSegment &segment = segments.emplace_back();
      segment.Start = pathStr.c_str() + start;
      segment.Length = pathStr.length() - start;
    }

    // Process each segment to check if it's an archive
    for (auto &handler : _archiveHandlers) {
      size_t endPos = _rootParentPathSize + 1;
      for (size_t i = 0; i < segments.size(); ++i) {
        endPos += segments[i].Length;
        if (!handler->TestPathSegment(&segments[i])) {
          ++endPos;
        } else {
          std::filesystem::path archivePath(pathStr.substr(0, endPos));
          std::filesystem::path archiveParentPath = archivePath.parent_path();

          ComObject<IMGDFReadOnlyFile> parent;
          // if the archive parent is above the root, then it should have a null
          // parent to prevent traversal outside the vfs root
          if (archiveParentPath.wstring().size() >=
              _rootPath.wstring().size()) {
            parent = MakeCom<DefaultReadOnlyFileImpl>(
                archiveParentPath.filename().c_str(), archiveParentPath,
                _rootPath, _archiveHandlers.data(), _archiveHandlers.size());
          }

          // Try to map archive with remaining path segments as logical path
          if (handler->MapArchive(
                  _rootPath.c_str(), archivePath.wstring().c_str(),
                  segments.data() + i + 1,  // segments after archive
                  segments.size() - i - 1, _archiveHandlers.data(),
                  _archiveHandlers.size(), parent, file)) {
            return TRUE;
          }
        }
      }
    }
  }
  auto node = MakeCom<DefaultReadOnlyFileImpl>(
      path.filename().c_str(), path, _rootPath, _archiveHandlers.data(),
      _archiveHandlers.size());
  node.AddRawRef(file);
  return TRUE;
}

void ReadOnlyVirtualFileSystemComponent::RegisterArchiveHandler(
    ComObject<IMGDFArchiveHandler> handler) {
  _ASSERTE(handler);
  handler->AddRef();
  _archiveHandlers.push_back(handler);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF