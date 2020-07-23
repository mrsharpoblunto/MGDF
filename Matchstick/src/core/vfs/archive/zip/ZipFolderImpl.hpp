#pragma once

#include "../../MGDFReadOnlyFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
implementation of a folder in a zipped archive
*/
class ZipFolderImpl : public ReadOnlyFileBaseImpl {
 public:
  ZipFolderImpl(const wchar_t *name, IMGDFReadOnlyFile *parent,
                IMGDFReadOnlyVirtualFileSystem *vfs, IMGDFReadOnlyFile *root)
      : ReadOnlyFileBaseImpl(parent, vfs), _name(name), _root(root) {}
  ~ZipFolderImpl(){};

  BOOL __stdcall IsArchive() final { return true; }
  BOOL __stdcall IsFolder() final { return true; }

  HRESULT __stdcall GetPhysicalName(wchar_t *name, UINT64 *length) final {
    return _root->GetPhysicalName(name, length);
  }

  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final {
    return _root->GetPhysicalPath(path, length);
  }

  HRESULT GetLogicalName(wchar_t *name, UINT64 *length) final;

  BOOL __stdcall IsOpen() final { return false; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final {
    (void)reader;
    return E_FAIL;
  }

  UINT64 __stdcall GetLastWriteTime() final {
    return _root->GetLastWriteTime();
  }

 private:
  IMGDFReadOnlyFile *_root;
  std::wstring _name;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF