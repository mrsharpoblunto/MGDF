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
};

struct ZipFileData {
  INT64 readPosition;
  char *data;
};

class ZipFileImpl;

class ZipFileImplReader : public ComBase<IFileReader> {
 public:
  ZipFileImplReader(ZipFileImpl *zip, const ZipFileHeader &header,
                    const ZipFileData &data);
  virtual ~ZipFileImplReader();
  UINT32 Read(void *buffer, UINT32 length) override final;
  void SetPosition(INT64 pos) override final;
  INT64 GetPosition() const override final;
  bool EndOfFile() const override final;
  INT64 GetSize() const override { return _size; }

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
  ZipFileImpl(IFile *parent, const IFile *root, unzFile zip,
              ZipFileHeader &&header)
      : FileBaseImpl(parent),
        _root(root),
        _header(header),
        _zip(zip),
        _reader(nullptr) {}
  virtual ~ZipFileImpl();

  bool IsFolder() const override final { return false; }
  bool IsArchive() const override final { return true; }

  bool IsOpen() const override final { return _reader; }

  HRESULT Open(IFileReader **reader) override final;

  time_t GetLastWriteTime() const override final {
    return _root->GetLastWriteTime();
  }
  const wchar_t *GetArchiveName() const override final {
    return _root->GetName();
  }
  const wchar_t *GetPhysicalPath() const override final {
    return _root->GetPhysicalPath();
  }
  const wchar_t *GetName() const override final { return _header.name.c_str(); }

 private:
  const IFile *_root;
  ZipFileHeader _header;
  IFileReader *_reader;
  unzFile _zip;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF