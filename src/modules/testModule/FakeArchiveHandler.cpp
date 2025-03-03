#include "StdAfx.h"

#include "FakeArchiveHandler.hpp"

#include <algorithm>
#include <filesystem>

#include "FakeFile.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace Test {

static const std::wstring FAKE_EXT(L".fakearchive");

FakeArchiveHandler::FakeArchiveHandler(IMGDFLogger *logger) : _logger(logger) {}

FakeArchiveHandler::~FakeArchiveHandler() {}

BOOL __stdcall FakeArchiveHandler::TestPathSegment(
    const MGDFArchivePathSegment *segment) {
  const std::wstring_view view(segment->Start, segment->Length);
  return view.ends_with(FAKE_EXT);
}

BOOL __stdcall FakeArchiveHandler::MapArchive(
    const wchar_t *rootPath, const wchar_t *archivePath,
    const MGDFArchivePathSegment *logicalPathSegments,
    UINT64 logicalPathSegmentCount, IMGDFArchiveHandler **handlers,
    UINT64 handlerCount, IMGDFReadOnlyFile *parent, IMGDFReadOnlyFile **file) {
  std::ignore = handlers;
  std::ignore = handlerCount;

  const auto root = std::filesystem::path(rootPath).lexically_normal();
  const auto physicalPath =
      std::filesystem::path(archivePath).lexically_normal();

  std::wstring logicalPath =
      physicalPath.wstring().substr(root.wstring().size() + 1);
  std::replace(logicalPath.begin(), logicalPath.end(), '\\', '/');

  ComObject<FakeContext> context(new FakeContext());
  context->Parent = ComObject<IMGDFReadOnlyFile>(parent, true);

  std::shared_ptr<FakeFile> rootFile =
      std::make_shared<FakeFile>(physicalPath.filename().wstring(),
                                 physicalPath.wstring(), logicalPath, context);
  context->Resources.push_back(rootFile);

  std::shared_ptr<FakeFile> subFile = std::make_shared<FakeFile>(
      L"testfile.txt", rootFile.get(), "hello world", context);
  rootFile->AddChild(subFile);

  ComObject<IMGDFReadOnlyFile> current(rootFile.get(), true);
  for (UINT64 i = 0; i < logicalPathSegmentCount; ++i) {
    std::wstring childName(logicalPathSegments[i].Start,
                           logicalPathSegments[i].Length);
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

}  // namespace Test
}  // namespace MGDF