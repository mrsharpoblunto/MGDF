#pragma once

#include "../../MGDFFileBaseImpl.hpp"
#include "ZipArchive.hpp"

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

/**
implementation of a file in a zipped archive
*/
class ZipFileImpl : public FileBaseImpl, public IFileReader {
 public:
  ZipFileImpl(IFile *parent, ZipArchive *handler, ZipFileHeader &&header)
      : FileBaseImpl(parent),
        _handler(handler),
        _header(header),
        _isOpen(false) {}
  virtual ~ZipFileImpl();

  bool IsFolder() const override final { return false; }
  bool IsArchive() const override final { return true; }

  bool IsOpen() const override final {
    std::lock_guard<std::mutex> lock(_mutex);
    return _isOpen;
  }

  MGDFError Open(IFileReader **reader) override final;
  void Close() override final;
  UINT32 Read(void *buffer, UINT32 length) override final;
  void SetPosition(INT64 pos) override final;
  INT64 GetPosition() const override final;
  bool EndOfFile() const override final;
  INT64 GetSize() const override { return _header.size; }

  time_t GetLastWriteTime() const override final {
    return _handler->GetArchiveRoot()->GetLastWriteTime();
  }
  const wchar_t *GetArchiveName() const override final {
    return _handler->GetArchiveRoot()->GetName();
  }
  const wchar_t *GetPhysicalPath() const override final {
    return _handler->GetArchiveRoot()->GetPhysicalPath();
  }
  const wchar_t *GetName() const override final { return _header.name.c_str(); }

 private:
  ZipArchive *_handler;
  ZipFileHeader _header;
  ZipFileData _data;
  bool _isOpen;
};

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF