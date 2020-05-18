#pragma once

#include "MGDFFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class FolderBaseImpl : public FileBaseImpl {
 public:
  FolderBaseImpl(const wchar_t *name, const wchar_t *physicalPath,
                 IFile *parent)
      : FileBaseImpl(parent), _name(name), _path(physicalPath) {
    _ASSERTE(name);
    _ASSERTE(physicalPath);
  }

  FolderBaseImpl(const std::wstring &name, const wchar_t *physicalPath,
                 IFile *parent)
      : FileBaseImpl(parent), _name(name), _path(physicalPath) {
    _ASSERTE(physicalPath);
  }

  FolderBaseImpl(const std::wstring &name, const std::wstring &physicalPath,
                 IFile *parent)
      : FileBaseImpl(parent), _name(name), _path(physicalPath) {}

  virtual ~FolderBaseImpl() {}

  bool FolderBaseImpl::IsOpen() const override final { return false; }
  HRESULT FolderBaseImpl::Open(IFileReader **reader) override final {
    return E_FAIL;
  }

  bool FolderBaseImpl::IsFolder() const override final { return true; }
  bool FolderBaseImpl::IsArchive() const override { return false; }
  const wchar_t *FolderBaseImpl::GetArchiveName() const override {
    return nullptr;
  }
  const wchar_t *FolderBaseImpl::GetPhysicalPath() const override final {
    return _path.c_str();
  }
  const wchar_t *GetName() const override final { return _name.c_str(); }

 private:
  std::wstring _name, _path;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF