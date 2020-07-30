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

/**
 abstract class which contains the common functionality to default file
 instances aswell as the zip and other archive file implementations of the
 standard ifile interface
*/
class FakeFile : public IMGDFReadOnlyFile, public IMGDFFileReader {
 public:
  FakeFile(const std::wstring &name, const std::wstring &physicalFile,
           IMGDFReadOnlyFile *parent, IMGDFReadOnlyVirtualFileSystem *vfs);
  FakeFile(const std::wstring &name, FakeFile *parent, const std::string &data);
  void AddChild(ComObject<FakeFile> file);
  virtual ~FakeFile(void);

  BOOL __stdcall GetParent(IMGDFReadOnlyFile **parent) final;
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;
  void __stdcall GetAllChildren(IMGDFReadOnlyFile **childBuffer) final;
  UINT64 __stdcall GetChildCount() final;
  void __stdcall GetVFS(IMGDFReadOnlyVirtualFileSystem **vfs) final;
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
    ComObject<FakeFile> Ref;
    std::wstring Name;
  };

  mutable std::mutex _mutex;
  mutable std::wstring _logicalPath;

  std::unique_ptr<std::map<const wchar_t *, FakeFile::ChildFileRef, WCharCmp>>
      _children;
  IMGDFReadOnlyFile *_parent;
  IMGDFReadOnlyVirtualFileSystem *_vfs;
  std::wstring _name;
  std::wstring _physicalPath;

  std::string _data;
  INT32 _position;
  bool _isOpen;

 private:
  std::atomic<ULONG> _references;
};

}  // namespace Test
}  // namespace MGDF