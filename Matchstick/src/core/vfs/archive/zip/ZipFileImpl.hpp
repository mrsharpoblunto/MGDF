#pragma once

#include <unzip.h>

#include <string>

#include "../../MGDFFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

struct ZipFileHeader {
  unz_file_pos filePosition;
  INT64 size;
  std::wstring name;

  ZipFileHeader() : size(0) {
    SecureZeroMemory(&filePosition, sizeof(unz_file_pos));
  }
};

struct ZipFileData {
  INT64 readPosition;
  char *data;
};

class ZipFileImpl;

class ZipFileImplReader : public ComBase<IMGDFFileReader> {
 public:
  ZipFileImplReader(ZipFileImpl *zip, const ZipFileHeader &header,
                    const ZipFileData &data);
  ~ZipFileImplReader();

  UINT32 __stdcall Read(void *buffer, UINT32 length) final;
  void __stdcall SetPosition(INT64 pos) final;
  INT64 __stdcall GetPosition() final;
  BOOL __stdcall  EndOfFile() final;
  INT64 __stdcall GetSize() override { return _size; }

 private:
  INT64 _readPosition;
  char *_data;
  INT64 _size;
  ComObject<ZipFileImpl> _zip;
};
/**
implementation of a file in a zipped archive
*/
class ZipFileImpl : public FileBaseImpl {
  friend class ZipFileImplReader;

 public:
  ZipFileImpl(IMGDFFile *parent, IMGDFFile *root, unzFile zip,
              ZipFileHeader &&header)
      : FileBaseImpl(parent),
        _root(root),
        _header(header),
        _zip(zip),
        _reader(nullptr) {}
  virtual ~ZipFileImpl();

  BOOL __stdcall IsFolder() final { return false; }
  BOOL __stdcall IsArchive() final { return true; }
  BOOL __stdcall IsOpen() final { return _reader!=nullptr; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;
  UINT64 __stdcall GetLastWriteTime() final { return _root->GetLastWriteTime(); }
  const wchar_t * __stdcall GetArchiveName() final { return _root->GetName(); }
  const wchar_t * __stdcall GetPhysicalPath() final {
    return _root->GetPhysicalPath();
  }
  const wchar_t * __stdcall GetName() final { return _header.name.c_str(); }

 private:
  IMGDFFile *_root;
  ZipFileHeader _header;
  IMGDFFileReader *_reader;
  unzFile _zip;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF