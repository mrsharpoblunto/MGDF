#pragma once

#include <unzip.h>

#include <string>

#include "../../MGDFReadOnlyFileBaseImpl.hpp"

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
class ZipFileImpl : public ReadOnlyFileBaseImpl {
  friend class ZipFileImplReader;

 public:
  ZipFileImpl(IMGDFReadOnlyFile *parent, IMGDFReadOnlyVirtualFileSystem *vfs,
              IMGDFReadOnlyFile *root, unzFile zip, ZipFileHeader &&header)
      : ReadOnlyFileBaseImpl(parent, vfs),
        _root(root),
        _header(header),
        _zip(zip),
        _reader(nullptr) {}
  virtual ~ZipFileImpl();

  BOOL __stdcall IsArchive() final { return true; }
  BOOL __stdcall IsFolder() final { return false; }

  HRESULT __stdcall GetPhysicalName(wchar_t *name, UINT64 *length) final {
    return _root->GetPhysicalName(name, length);
  }

  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final {
    return _root->GetPhysicalPath(path, length);
  }

  HRESULT __stdcall GetLogicalName(wchar_t *name, UINT64 *length) final;

  BOOL __stdcall IsOpen() final { return _reader != nullptr; }
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;

  UINT64 __stdcall GetLastWriteTime() final {
    return _root->GetLastWriteTime();
  }

 private:
  IMGDFReadOnlyFile *_root;
  ZipFileHeader _header;
  IMGDFFileReader *_reader;
  unzFile _zip;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF