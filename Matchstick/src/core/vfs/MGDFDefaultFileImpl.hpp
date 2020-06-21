#pragma once

#include <fstream>
#include <iostream>

#include "MGDFFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class DefaultFileImpl;

class DefaultFileReader : public ComBase<IMGDFFileReader> {
 public:
  DefaultFileReader(DefaultFileImpl *parent,
                    std::shared_ptr<std::ifstream> stream);
  virtual ~DefaultFileReader();
  UINT32 __stdcall Read(void *buffer, UINT32 length) final;
  void __stdcall SetPosition(INT64 pos) final;
  INT64 __stdcall GetPosition() final;
  BOOL __stdcall EndOfFile() final;
  INT64 __stdcall GetSize() final;

 private:
  ComObject<DefaultFileImpl> _parent;
  INT64 _fileSize;
  std::shared_ptr<std::ifstream> _stream;
};

class DefaultFileImpl : public FileBaseImpl {
  friend class DefaultFileReader;

 public:
  DefaultFileImpl(const std::wstring &name, const std::wstring &physicalPath,
                  IMGDFFile *parent);
  virtual ~DefaultFileImpl();

  BOOL __stdcall IsOpen() final { return _reader!=nullptr; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;
  BOOL __stdcall IsFolder() final { return false; }
  BOOL __stdcall IsArchive() override { return false; }
  const wchar_t * __stdcall GetArchiveName() override { return nullptr; }
  const wchar_t * __stdcall GetPhysicalPath() final { return _path.c_str(); }
  const wchar_t * __stdcall GetName() final { return _name.c_str(); }

 private:
  std::wstring _name;
  std::wstring _path;
  DefaultFileReader *_reader;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
