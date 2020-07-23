#pragma once

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>

#include "MGDFDefaultFileReader.hpp"
#include "MGDFWriteableVirtualFileSystem.hpp"

namespace MGDF {
namespace core {
namespace vfs {

class DefaultFileWriter : public ComBase<IMGDFFileWriter> {
 public:
  DefaultFileWriter(std::shared_ptr<std::ofstream> stream,
                    std::function<void()> cleanup);
  virtual ~DefaultFileWriter();
  UINT32 __stdcall Write(void *buffer, UINT32 length) final;
  void __stdcall SetPosition(INT64 pos) final;
  INT64 __stdcall GetPosition() final;

 private:
  std::function<void()> _cleanup;
  std::shared_ptr<std::ofstream> _stream;
};

class DefaultWriteableFileImpl : public ComBase<IMGDFWriteableFile, IMGDFFile> {
 public:
  DefaultWriteableFileImpl(const std::wstring &name,
                           const std::filesystem::path &physicalPath,
                           WriteableVirtualFileSystem *vfs)
      : _name(name),
        _physicalPath(physicalPath),
        _reader(nullptr),
        _writer(nullptr),
        _vfs(vfs) {}
  virtual ~DefaultWriteableFileImpl() {}

  BOOL __stdcall Exists() final;
  BOOL __stdcall IsFolder() final;
  BOOL __stdcall GetParent(IMGDFWriteableFile **parent) final;
  HRESULT __stdcall GetChild(const wchar_t *name,
                             IMGDFWriteableFile **child) final;
  HRESULT __stdcall GetAllChildren(IMGDFWriteableFile **childBuffer) final;
  HRESULT __stdcall OpenWrite(IMGDFFileWriter **writer) final;

  HRESULT __stdcall GetPhysicalName(wchar_t *name, UINT64 *length) final;
  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final;
  HRESULT __stdcall GetLogicalName(wchar_t *name, UINT64 *length) final;
  void __stdcall GetVFS(IMGDFWriteableVirtualFileSystem **vfs) final;
  HRESULT __stdcall GetLogicalPath(wchar_t *path, UINT64 *length) final;

  BOOL __stdcall IsOpen() final;
  HRESULT __stdcall Open(IMGDFFileReader **reader) final;
  UINT64 __stdcall GetChildCount() final;
  UINT64 __stdcall GetLastWriteTime() final;
  HRESULT __stdcall CreateFolder() final;
  HRESULT __stdcall Delete() final;
  HRESULT __stdcall MoveTo(IMGDFWriteableFile *destination) final;

 protected:
 private:
  std::mutex _mutex;
  std::wstring _name;
  std::filesystem::path _physicalPath;
  DefaultFileWriter *_writer;
  DefaultFileReader *_reader;
  WriteableVirtualFileSystem *_vfs;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
