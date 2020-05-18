#pragma once

#include <fstream>
#include <iostream>

#include "MGDFFileBaseImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class DefaultFileImpl;

class DefaultFileReader : public ComBase<IFileReader> {
 public:
  DefaultFileReader(DefaultFileImpl *parent,
                    std::shared_ptr<std::ifstream> stream);
  virtual ~DefaultFileReader();
  UINT32 Read(void *buffer, UINT32 length) override final;
  void SetPosition(INT64 pos) override final;
  INT64 GetPosition() const override final;
  bool EndOfFile() const override final;
  INT64 GetSize() const override final;

 private:
  ComObject<DefaultFileImpl> _parent;
  INT64 _fileSize;
  std::shared_ptr<std::ifstream> _stream;
};

class DefaultFileImpl : public FileBaseImpl {
  friend class DefaultFileReader;

 public:
  DefaultFileImpl(const std::wstring &name, const std::wstring &physicalPath,
                  IFile *parent);
  virtual ~DefaultFileImpl();

  bool IsOpen() const override final { return _reader; }

  HRESULT Open(IFileReader **reader) override final;

  bool IsFolder() const override final { return false; }
  bool IsArchive() const override { return false; }
  const wchar_t *GetArchiveName() const override { return nullptr; }
  const wchar_t *GetPhysicalPath() const override final {
    return _path.c_str();
  }
  const wchar_t *GetName() const override final { return _name.c_str(); }

 private:
  std::wstring _name;
  std::wstring _path;
  DefaultFileReader *_reader;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
