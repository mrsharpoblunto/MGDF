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
    const std::filesystem::path &rootPath)
    : DefaultFileBase(name, physicalPath, rootPath) {}

ComObject<IMGDFReadOnlyFile> DefaultReadOnlyFileImpl::CreateFile(
    const std::wstring &name, const std::filesystem::path &path,
    const std::filesystem::path &rootPath) {
  return MakeCom<DefaultReadOnlyFileImpl>(name, path, rootPath)
      .As<IMGDFReadOnlyFile>();
}

BOOL DefaultReadOnlyFileImpl::GetChild(const wchar_t *name,
                                       IMGDFReadOnlyFile **child) {
  const auto childPath = _physicalPath / name;
  if (!std::filesystem::exists(childPath)) {
    return FALSE;
  }
  auto childFile = MakeCom<DefaultReadOnlyFileImpl>(name, childPath, _rootPath);
  childFile.AddRawRef(child);
  return TRUE;
}

ReadOnlyVirtualFileSystemComponent::ReadOnlyVirtualFileSystemComponent() {}

bool ReadOnlyVirtualFileSystemComponent::Mount(
    const wchar_t *physicalDirectory) {
  _rootPath = std::filesystem::path(physicalDirectory).lexically_normal();
  return std::filesystem::is_directory(physicalDirectory);
}

BOOL ReadOnlyVirtualFileSystemComponent::GetFile(const wchar_t *logicalPath,
                                                 IMGDFReadOnlyFile **file) {
  if (!logicalPath) {
    GetRoot(file);
    return true;
  }

  std::filesystem::path path = (_rootPath / logicalPath).lexically_normal();
  if (!_archiveHandlers.empty()) {
    std::wstring pathStr = path.wstring();
    std::vector<MGDFArchivePathSegment> segments;

    // Split path into segments
    size_t start = 0;
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
    size_t endPos = 0;
    for (auto &handler : _archiveHandlers) {
      for (size_t i = 0; i < segments.size(); ++i) {
        endPos += segments[i].Length;
        if (!handler->TestPathSegment(&segments[i])) {
          ++endPos;
        } else {
          std::wstring archivePathStr = pathStr.substr(0, endPos);

          // Try to map archive with remaining path segments as logical path
          if (handler->MapArchive(
                  _rootPath.c_str(), archivePathStr.c_str(),
                  segments.data() + i + 1,  // segments after archive
                  segments.size() - i - 1, file)) {
            return true;
          }
        }
      }
    }
  }
  auto node = MakeCom<DefaultReadOnlyFileImpl>(path.filename().c_str(), path,
                                               _rootPath);
  node.AddRawRef(file);
  return true;
}

void ReadOnlyVirtualFileSystemComponent::RegisterArchiveHandler(
    ComObject<IMGDFArchiveHandler> handler) {
  _ASSERTE(handler);
  _archiveHandlers.push_back(handler);
}

void ReadOnlyVirtualFileSystemComponent::GetRoot(IMGDFReadOnlyFile **root) {
  auto r = MakeCom<DefaultReadOnlyFileImpl>(_rootPath.filename().c_str(),
                                            _rootPath, _rootPath);
  r.AddRawRef(root);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF