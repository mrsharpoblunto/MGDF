#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
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
           IMGDFReadOnlyFile *parent);
  FakeFile(const std::wstring &name, FakeFile *parent, const std::string &data);
  void AddChild(ComObject<FakeFile> file);
  virtual ~FakeFile(void);

  BOOL GetParent(IMGDFReadOnlyFile **parent) final;
  BOOL GetChild(const wchar_t *name, IMGDFReadOnlyFile **child) final;
  void GetAllChildren(IMGDFReadOnlyFile **childBuffer) final;
  UINT64 GetChildCount() final;

  HRESULT Open(IMGDFFileReader **reader) final;

  BOOL IsOpen() final;
  UINT32 Read(void *buffer, UINT32 length) final;
  void SetPosition(INT64 pos) final;
  INT64 GetPosition() final;
  BOOL EndOfFile() final;
  INT64 GetSize() final;

  BOOL IsFolder() final;
  BOOL IsArchive() final;
  const wchar_t *GetArchiveName() final;
  const wchar_t *GetPhysicalPath() final;
  const wchar_t *GetName() final;
  UINT64 GetLastWriteTime() final;

  ULONG AddRef() final;
  ULONG Release() final;
  HRESULT QueryInterface(REFIID riid, void **ppvObject) final;

 protected:
  mutable std::mutex _mutex;
  mutable std::wstring _logicalPath;

  std::unique_ptr<std::map<const wchar_t *, ComObject<FakeFile>, WCharCmp>>
      _children;
  IMGDFReadOnlyFile *_parent;
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