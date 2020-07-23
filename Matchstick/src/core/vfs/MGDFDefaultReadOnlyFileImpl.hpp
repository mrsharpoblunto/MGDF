#pragma once

#include <fstream>
#include <iostream>

#include "MGDFDefaultFileReader.hpp"
#include "MGDFReadOnlyFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class DefaultReadOnlyFileImpl : public ReadOnlyFileBaseImpl {
 public:
  DefaultReadOnlyFileImpl(const std::wstring &name,
                          const std::wstring &physicalPath,
                          IMGDFReadOnlyFile *parent,
                          IMGDFReadOnlyVirtualFileSystem *vfs)
      : _name(name),
        _path(physicalPath),
        _reader(nullptr),
        ReadOnlyFileBaseImpl(parent, vfs) {}
  virtual ~DefaultReadOnlyFileImpl() {}

  BOOL __stdcall IsFolder() override { return false; }
  BOOL __stdcall IsArchive() override { return false; }
  HRESULT
  __stdcall GetPhysicalName(wchar_t *name, UINT64 *length) final;
  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final;
  HRESULT
  __stdcall GetLogicalName(wchar_t *name, UINT64 *length) final;
  BOOL __stdcall IsOpen() final { return _reader != nullptr; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;
  UINT64 __stdcall GetLastWriteTime() final;

 protected:
  std::wstring _name;
  std::wstring _path;
  DefaultFileReader *_reader;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
