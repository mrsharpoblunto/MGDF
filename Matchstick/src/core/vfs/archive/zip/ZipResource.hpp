#pragma once

#include <MGDF/MGDF.h>
#include <minizip/unzip.h>

#include <MGDF/ComObject.hpp>
#include <filesystem>
#include <memory>
#include <string>

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

struct ZipContext;

class ZipResource : public IMGDFReadOnlyFile {
 public:
  ZipResource(const wchar_t *name, ZipResource *parent, ZipContext *context);
  virtual ~ZipResource() {}

  ULONG __stdcall AddRef() final;
  ULONG __stdcall Release() final;
  HRESULT __stdcall QueryInterface(REFIID riid, void **ppvObject) final;

  BOOL __stdcall IsArchive() final { return true; }
  HRESULT GetLogicalName(wchar_t *name, UINT64 *length) final;
  HRESULT __stdcall GetPhysicalName(wchar_t *name, UINT64 *length) final;
  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final;
  HRESULT __stdcall GetLogicalPath(wchar_t *path, UINT64 *length) final;
  BOOL GetParent(IMGDFReadOnlyFile **parent) final;
  UINT64 __stdcall GetLastWriteTime() final;

  const std::wstring &GetName() const { return _name; }

 protected:
  std::wstring _name;
  ZipResource *_parent;
  ZipContext *_context;
};

struct ZipContext : ComBase<IUnknown> {
 public:
  virtual ~ZipContext() { unzClose(Zip); }

  unzFile Zip = nullptr;
  uint64_t LastWriteTime = 0;
  std::wstring LogicalPath;
  std::filesystem::path PhysicalPath;
  std::vector<std::shared_ptr<ZipResource>> Resources;
  ComObject<IMGDFReadOnlyFile> Parent;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF
