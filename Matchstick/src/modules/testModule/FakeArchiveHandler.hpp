#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <atomic>
#include <string>
#include <vector>

namespace MGDF {
namespace Test {

class FakeArchiveHandler : public ComBase<MGDF::IArchiveHandler> {
 public:
  FakeArchiveHandler(MGDF::ILogger *logger);
  virtual ~FakeArchiveHandler();
  virtual bool IsArchive(const wchar_t *path) const final;
  virtual HRESULT MapArchive(const wchar_t *name, const wchar_t *archiveFile,
                             MGDF::IFile *parent, MGDF::IFile **child) final;

 private:
  std::vector<const wchar_t *> _fileExtensions;
  MGDF::ILogger *_logger;
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