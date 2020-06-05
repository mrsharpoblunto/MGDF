#pragma once

#include <unzip.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <map>
#include <string>
#include <vector>

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
Creates zip archive handlers
*/
class ZipArchiveHandlerImpl : public ComBase<IArchiveHandler> {
 public:
  ZipArchiveHandlerImpl();
  ~ZipArchiveHandlerImpl();
  bool IsArchive(const wchar_t *physicalPath) const final;
  HRESULT MapArchive(const wchar_t *name, const wchar_t *physicalPath,
                     IFile *parent, IFile **file) final;

 private:
  std::vector<const wchar_t *> _fileExtensions;

  /**
  get the extension of a file
  \return the extension (excluding the preceding '.' if possible, otherwise
  returns "" if no extension could be found
  */
  const wchar_t *GetFileExtension(const wchar_t *file) const;

  ComObject<IFile> CreateParentFile(std::wstring &path, ComObject<IFile> root,
                                    const wchar_t **);
};

ComObject<IArchiveHandler> CreateZipArchiveHandlerImpl();

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF