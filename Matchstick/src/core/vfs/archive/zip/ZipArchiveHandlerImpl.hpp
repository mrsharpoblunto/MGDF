#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "ZipArchive.hpp"
#include "ZipFolderImpl.hpp"

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

  BOOL __stdcall TestPathSegment(const MGDFArchivePathSegment *segment) final;
  BOOL __stdcall MapArchive(const wchar_t *rootPath, const wchar_t *fullPath,
                            const MGDFArchivePathSegment *segments,
                            UINT64 segmentCount, IMGDFArchiveHandler **handlers,
                            UINT64 handlerCount, IMGDFReadOnlyFile *parent,
                            IMGDFReadOnlyFile **file) final;

 private:
  ZipFolderImpl *CreateParentFolder(std::wstring &path, ZipArchive *archive,
                                    ZipFolderImpl *root,
                                    const wchar_t **filename);
};

ComObject<IMGDFArchiveHandler> CreateZipArchiveHandlerImpl();

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF