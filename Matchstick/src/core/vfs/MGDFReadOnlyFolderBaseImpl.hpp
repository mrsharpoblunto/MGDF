#pragma once

#include "MGDFReadOnlyFileBaseImpl.hpp"
#include "MGDFReadOnlyFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class ReadOnlyFolderBaseImpl : public ReadOnlyFileBaseImpl {
 public:
  ReadOnlyFolderBaseImpl(const wchar_t *name, const wchar_t *physicalPath,
                 IMGDFReadOnlyFile *parent)
      : ReadOnlyFileBaseImpl(parent), _name(name), _path(physicalPath) {
    _ASSERTE(name);
    _ASSERTE(physicalPath);
  }

  ReadOnlyFolderBaseImpl(const std::wstring &name, const wchar_t *physicalPath,
                 IMGDFReadOnlyFile *parent)
      : ReadOnlyFileBaseImpl(parent), _name(name), _path(physicalPath) {
    _ASSERTE(physicalPath);
  }

  ReadOnlyFolderBaseImpl(const std::wstring &name, const std::wstring &physicalPath,
                 IMGDFReadOnlyFile *parent)
      : ReadOnlyFileBaseImpl(parent), _name(name), _path(physicalPath) {}

  virtual ~ReadOnlyFolderBaseImpl() {}

  BOOL __stdcall IsOpen() final { return false; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final {
    (void)reader;
    return E_FAIL;
  }

  BOOL __stdcall IsFolder() final { return true; }
  BOOL __stdcall IsArchive() override { return false; }
  const wchar_t * __stdcall GetArchiveName() override {
    return nullptr;
  }
  const wchar_t * __stdcall GetPhysicalPath() final {
    return _path.c_str();
  }
  const wchar_t * __stdcall GetName() final { return _name.c_str(); }

 private:
  std::wstring _name, _path;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF