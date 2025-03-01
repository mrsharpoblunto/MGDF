#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "MGDFDefaultFileBase.hpp"

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

class DefaultWriteableFileImpl : public DefaultFileBase<IMGDFWriteableFile> {
 public:
  DefaultWriteableFileImpl(const std::wstring &name,
                           const std::filesystem::path &physicalPath,
                           std::shared_ptr<DefaultFileBaseContext> context);
  virtual ~DefaultWriteableFileImpl() {}

  BOOL __stdcall IsOpen() final;
  UINT64 __stdcall GetChildCount() final;
  BOOL GetChild(const wchar_t *name, IMGDFWriteableFile **child) final;
  HRESULT __stdcall GetAllChildren(IMGDFWriteableFile **childBuffer,
                                   UINT64 *length) final;

  BOOL __stdcall Exists() final;
  HRESULT __stdcall OpenWrite(IMGDFFileWriter **writer) final;
  HRESULT __stdcall CreateFolder() final;
  HRESULT __stdcall Delete() final;
  HRESULT __stdcall MoveTo(IMGDFWriteableFile *destination) final;

  ComObject<IMGDFWriteableFile> CreateFile(
      const std::wstring &name, const std::filesystem::path &path) final;

 private:
  DefaultFileWriter *_writer;
};

class WriteableVirtualFileSystem
    : public ComBase<IMGDFWriteableVirtualFileSystem> {
 public:
  virtual ~WriteableVirtualFileSystem() {}
  WriteableVirtualFileSystem(const std::wstring &rootPath);

  BOOL __stdcall GetFile(const wchar_t *logicalPath,
                         IMGDFWriteableFile **file) final;
  void __stdcall GetRoot(IMGDFWriteableFile **root) final;

  const std::filesystem::path &GetRootPath() const {
    return _context->RootPath;
  }

 private:
  std::shared_ptr<DefaultFileBaseContext> _context;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
