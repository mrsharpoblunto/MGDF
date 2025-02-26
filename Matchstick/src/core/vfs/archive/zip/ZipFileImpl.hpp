#pragma once

#include <minizip/unzip.h>

#include <MGDF/ComObject.hpp>
#include <mutex>
#include <string>

#include "ZipArchive.hpp"

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
  BOOL __stdcall EndOfFile() final;
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
class ZipFileImpl : public ZipResource {
  friend class ZipFileImplReader;

 public:
  ZipFileImpl(const wchar_t *name, ZipResource *parent, ZipArchive *archive,
              ZipFileHeader &&header)
      : ZipResource(name, parent, archive), _header(header), _reader(nullptr) {}
  virtual ~ZipFileImpl();

  BOOL __stdcall IsFolder() final { return false; }
  BOOL __stdcall IsOpen() final;
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;
  HRESULT __stdcall GetAllChildren(IMGDFReadOnlyFile **buffer,
                                   UINT64 *length) final;
  UINT64 __stdcall GetChildCount() final { return 0U; }
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;
  HRESULT __stdcall CopyTo(IMGDFWriteableFile *destination) final;

 private:
  std::mutex _mutex;
  ZipFileHeader _header;
  IMGDFFileReader *_reader;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF