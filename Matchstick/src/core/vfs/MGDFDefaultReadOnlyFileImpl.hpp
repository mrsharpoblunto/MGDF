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
                          IMGDFReadOnlyFile *parent)
      : _name(name),
        _physicalPath(physicalPath),
        _reader(nullptr),
        ReadOnlyFileBaseImpl(parent) {}
  virtual ~DefaultReadOnlyFileImpl() {}

  BOOL __stdcall IsFolder() final { return false; }
  BOOL __stdcall IsArchive() override { return false; }
  const wchar_t *__stdcall GetArchiveName() override { return nullptr; }
  const wchar_t *__stdcall GetName() final { return _name.c_str(); }
  const wchar_t *__stdcall GetPhysicalPath() final {
    return _physicalPath.c_str();
  }
  BOOL __stdcall IsOpen() final { return _reader!=nullptr; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;

 private:
  std::wstring _name;
  std::wstring _physicalPath;
  DefaultFileReader *_reader;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
