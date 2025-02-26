#pragma once

#include <MGDF/MGDF.h>

#include <unordered_map>

#include "../../../common/MGDFStringImpl.hpp"
#include "../../MGDFReadOnlyVirtualFileSystemComponent.hpp"
#include "ZipArchive.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
implementation of a folder in a zipped archive
*/
class ZipFolderImpl : public ZipResource {
 public:
  ZipFolderImpl(const wchar_t *name, ZipFolderImpl *parent, ZipArchive *archive)
      : ZipResource(name, parent, archive) {}
  ~ZipFolderImpl() {};

  BOOL __stdcall IsFolder() final { return true; }
  BOOL __stdcall IsOpen() final { return false; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;
  HRESULT __stdcall GetAllChildren(IMGDFReadOnlyFile **buffer,
                                   UINT64 *length) final;
  UINT64 __stdcall GetChildCount() final;
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;
  HRESULT __stdcall CopyTo(IMGDFWriteableFile *destination) final;

  void AddChild(std::shared_ptr<ZipResource> child);

 private:
  std::unordered_map<std::wstring, ZipResource *> _children;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF