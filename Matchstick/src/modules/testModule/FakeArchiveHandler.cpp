#include "StdAfx.h"

#include "FakeArchiveHandler.hpp"

#include "FakeFile.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace Test {

static const wchar_t *FAKE_EXT = L".fakearchive";

FakeArchiveHandler::FakeArchiveHandler(MGDF::ILogger *logger,
                                       MGDF::IErrorHandler *errorHandler)
    : _logger(logger), _errorHandler(errorHandler), _references(1UL) {
  _fileExtensions.push_back(FAKE_EXT);
}

FakeArchiveHandler::~FakeArchiveHandler() {}

HRESULT FakeArchiveHandler::MapArchive(const wchar_t *name,
                                       const wchar_t *archiveFile,
                                       MGDF::IFile *parent,
                                       MGDF::IFile **child) {
  ComObject<FakeFile> rootFile(new FakeFile(name, archiveFile, parent));

  ComObject<FakeFile> subFile(
      new FakeFile(L"testfile.txt", rootFile, "hello world"));
  rootFile->AddChild(subFile);

  rootFile.AddRawRef(child);
  return S_OK;
}

bool FakeArchiveHandler::IsArchive(const wchar_t *path) const {
  _ASSERTE(path);
  const wchar_t *extension = GetFileExtension(path);
  if (!extension) return false;

  for (auto ext : _fileExtensions) {
    if (wcscmp(ext, extension) == 0) {
      return true;
    }
  }
  return false;
}

const wchar_t *FakeArchiveHandler::GetFileExtension(
    const wchar_t *filename) const {
  _ASSERTE(filename);
  size_t index = wcslen(filename);
  while (index >= 0) {
    if (filename[index] == '.') return &filename[index];
    --index;
  }
  return nullptr;
}

}  // namespace Test
}  // namespace MGDF