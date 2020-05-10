#pragma once

#include <fstream>
#include <iostream>

#include "MGDFFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class DefaultFileImpl : public FileBaseImpl, public IFileReader {
 public:
  DefaultFileImpl(const std::wstring &name, const std::wstring &physicalPath,
                  IFile *parent, IErrorHandler *handler);
  virtual ~DefaultFileImpl();

  bool IsOpen() const override final {
    std::lock_guard<std::mutex> lock(_mutex);
    return (_fileStream != nullptr);
  }

  MGDFError Open(IFileReader **reader) override final;

  bool IsFolder() const override final { return false; }
  bool IsArchive() const override { return false; }
  const wchar_t *GetArchiveName() const override { return nullptr; }
  const wchar_t *GetPhysicalPath() const override final {
    return _path.c_str();
  }
  const wchar_t *GetName() const override final { return _name.c_str(); }

  void Close() override final;
  UINT32 Read(void *buffer, UINT32 length) override final;
  void SetPosition(INT64 pos) override final;
  INT64 GetPosition() const override final;
  bool EndOfFile() const override final;
  INT64 GetSize() const override final;

 private:
  std::ifstream *_fileStream;
  INT64 _filesize;
  std::wstring _name;
  std::wstring _path;
  IErrorHandler *_errorHandler;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
