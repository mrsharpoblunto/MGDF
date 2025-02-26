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

class ZipArchive;

class ZipResource : public IMGDFReadOnlyFile {
 public:
  ZipResource(const wchar_t *name, ZipResource *parent, ZipArchive *archive)
      : _name(name), _parent(parent), _archive(archive) {}
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
  ZipArchive *_archive;
  ZipResource *_parent;
};

class ZipArchive : public ComBase<IUnknown> {
 public:
  ZipArchive(const std::wstring &logicalPath, const std::wstring &physicalPath,
             uint64_t lastWriteTime, unzFile zip)
      : _zip(zip),
        _lastWriteTime(lastWriteTime),
        _logicalPath(logicalPath),
        _physicalPath(physicalPath) {}
  ~ZipArchive() { unzClose(_zip); }

  unzFile GetZip() { return _zip; }
  uint64_t GetLastWriteTime() const { return _lastWriteTime; }
  const std::wstring &GetLogicalPath() const { return _logicalPath; }
  const std::filesystem::path &GetPhysicalPath() const { return _physicalPath; }
  void AddResource(std::shared_ptr<ZipResource> resource) {
    _resources.push_back(resource);
  }

 private:
  unzFile _zip;
  uint64_t _lastWriteTime;
  std::wstring _logicalPath;
  std::filesystem::path _physicalPath;
  std::vector<std::shared_ptr<ZipResource>> _resources;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF
