#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <map>
#include <mutex>
#include <string>

namespace MGDF {
namespace Test {

struct WCharCmp {
  bool operator()(const wchar_t *a, const wchar_t *b) const {
    return std::wcscmp(a, b) < 0;
  }
};

class FakeFile;

struct FakeContext : ComBase<IUnknown> {
 public:
  virtual ~FakeContext() {}

  std::vector<std::shared_ptr<FakeFile>> Resources;
  ComObject<IMGDFReadOnlyFile> Parent;
};

/**
 abstract class which contains the common functionality to default file
 instances aswell as the zip and other archive file implementations of the
 standard ifile interface
*/
class FakeFile : public IMGDFReadOnlyFile, public IMGDFFileReader {
 public:
  FakeFile(const std::wstring &name, const std::wstring &physicalPath,
           const std::wstring &logicalPath, FakeContext *context);
  FakeFile(const std::wstring &name, FakeFile *parent, const std::string &data,
           FakeContext *context);
  void AddChild(std::shared_ptr<FakeFile> file);
  virtual ~FakeFile(void);

  BOOL __stdcall GetParent(IMGDFReadOnlyFile **parent) final;
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;
  HRESULT __stdcall GetAllChildren(IMGDFReadOnlyFile **childBuffer,
                                   UINT64 *length) final;
  UINT64 __stdcall GetChildCount() final;
  HRESULT __stdcall GetLogicalPath(wchar_t *path, UINT64 *length) final;
  HRESULT __stdcall CopyTo(IMGDFWriteableFile *destination) final;

  HRESULT __stdcall Open(IMGDFFileReader **reader) final;

  BOOL __stdcall IsOpen() final;
  UINT32 __stdcall Read(void *buffer, UINT32 length) final;
  void __stdcall SetPosition(INT64 pos) final;
  INT64 __stdcall GetPosition() final;
  BOOL __stdcall EndOfFile() final;
  INT64 __stdcall GetSize() final;

  BOOL __stdcall IsFolder() final;
  BOOL __stdcall IsArchive() final;

  HRESULT __stdcall GetLogicalName(wchar_t *path, UINT64 *length) final;
  HRESULT __stdcall GetPhysicalPath(wchar_t *path, UINT64 *length) final;
  HRESULT __stdcall GetPhysicalName(wchar_t *path, UINT64 *length) final;

  UINT64 GetLastWriteTime() final;

  ULONG AddRef() final;
  ULONG Release() final;
  HRESULT QueryInterface(REFIID riid, void **ppvObject) final;

 protected:
  struct ChildFileRef {
    FakeFile *Ref = nullptr;
    std::wstring Name;
  };

  mutable std::mutex _mutex;
  mutable std::wstring _logicalPath;

  std::unique_ptr<std::map<const wchar_t *, FakeFile::ChildFileRef, WCharCmp>>
      _children;
  FakeContext *_context;
  FakeFile *_parent;
  std::wstring _name;
  std::wstring _physicalPath;

  std::string _data;
  INT32 _position;
  bool _isOpen;
};

}  // namespace Test
}  // namespace MGDF