#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <string>
#include <vector>

namespace MGDF {
namespace Test {

class FakeArchiveHandler : public ComBase<IMGDFArchiveHandler> {
 public:
  FakeArchiveHandler(IMGDFLogger *logger);
  ~FakeArchiveHandler();
  BOOL IsArchive(const wchar_t *path) final;
  HRESULT MapArchive(const wchar_t *name, const wchar_t *archiveFile,
                     IMGDFReadOnlyFile *parent,
                     IMGDFReadOnlyVirtualFileSystem *vfs,
                     IMGDFReadOnlyFile **child) final;

 private:
  std::vector<const wchar_t *> _fileExtensions;
  IMGDFLogger *_logger;
  std::atomic<ULONG> _references;

  /**
  get the extension of a file
  \return the extension (excluding the preceding '.' if possible, otherwise
  returns "" if no extension could be found
  */
  const wchar_t *GetFileExtension(const wchar_t *file) const;
};

}  // namespace Test
}  // namespace MGDF