#pragma once

#include "MGDFFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class FolderBaseImpl : public FileBaseImpl {
 public:
  FolderBaseImpl(const wchar_t *name, const wchar_t *physicalPath,
                 IMGDFFile *parent)
      : FileBaseImpl(parent), _name(name), _path(physicalPath) {
    _ASSERTE(name);
    _ASSERTE(physicalPath);
  }

  FolderBaseImpl(const std::wstring &name, const wchar_t *physicalPath,
                 IMGDFFile *parent)
      : FileBaseImpl(parent), _name(name), _path(physicalPath) {
    _ASSERTE(physicalPath);
  }

  FolderBaseImpl(const std::wstring &name, const std::wstring &physicalPath,
                 IMGDFFile *parent)
      : FileBaseImpl(parent), _name(name), _path(physicalPath) {}

  virtual ~FolderBaseImpl() {}

  BOOL __stdcall FolderBaseImpl::IsOpen() final { return false; }
  HRESULT __stdcall FolderBaseImpl::Open(IMGDFFileReader **reader) final {
    (void)reader;
    return E_FAIL;
  }

  BOOL __stdcall FolderBaseImpl::IsFolder() final { return true; }
  BOOL __stdcall FolderBaseImpl::IsArchive() override { return false; }
  const wchar_t * __stdcall FolderBaseImpl::GetArchiveName() override {
    return nullptr;
  }
  const wchar_t * __stdcall FolderBaseImpl::GetPhysicalPath() final {
    return _path.c_str();
  }
  const wchar_t * __stdcall GetName() final { return _name.c_str(); }

 private:
  std::wstring _name, _path;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF