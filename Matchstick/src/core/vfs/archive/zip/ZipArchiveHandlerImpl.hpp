#pragma once

#include <unzip.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
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
class ZipArchiveHandlerImpl : public ComBase<IMGDFArchiveHandler> {
 public:
  ZipArchiveHandlerImpl();
  ~ZipArchiveHandlerImpl();
  
  BOOL __stdcall IsArchive(const wchar_t *physicalPath) final;
  HRESULT __stdcall MapArchive(const wchar_t *name, const wchar_t *physicalPath,
                     IMGDFFile *parent, IMGDFFile **file) final;

 private:
  std::vector<const wchar_t *> _fileExtensions;

  /**
  get the extension of a file
  \return the extension (excluding the preceding '.' if possible, otherwise
  returns "" if no extension could be found
  */
  const wchar_t *GetFileExtension(const wchar_t *file) const;

  ComObject<IMGDFFile> CreateParentFile(std::wstring &path, ComObject<IMGDFFile> root,
                                    const wchar_t **);
};

ComObject<IMGDFArchiveHandler> CreateZipArchiveHandlerImpl();

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF