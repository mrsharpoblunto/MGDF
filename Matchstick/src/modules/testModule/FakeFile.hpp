#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
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
class FakeFile : public MGDF::IFile, public MGDF::IFileReader {
 public:
  FakeFile(const std::wstring &name, const std::wstring &physicalFile,
           MGDF::IFile *parent);
  FakeFile(const std::wstring &name, FakeFile *parent, const std::string &data);
  void AddChild(ComObject<FakeFile> file);
  virtual ~FakeFile(void);

  bool GetParent(IFile **parent) override final;
  bool GetChild(const wchar_t *name, IFile **child) override final;
  void GetAllChildren(IFile **childBuffer) override final;
  size_t GetChildCount() override final;
  const wchar_t *GetLogicalPath() override final;

  HRESULT Open(IFileReader **reader) override final;

  bool IsOpen() const override final;
  UINT32 Read(void *buffer, UINT32 length) override final;
  void SetPosition(INT64 pos) override final;
  INT64 GetPosition() const override final;
  bool EndOfFile() const override final;
  INT64 GetSize() const override final;

  bool IsFolder() const override final;
  bool IsArchive() const override final;
  const wchar_t *GetArchiveName() const override final;
  const wchar_t *GetPhysicalPath() const override final;
  const wchar_t *GetName() const override final;
  time_t GetLastWriteTime() const override final;

  ULONG AddRef() override;
  ULONG Release() override;
  HRESULT QueryInterface(REFIID riid, void **ppvObject) override;

 protected:
  mutable std::mutex _mutex;
  mutable std::wstring _logicalPath;

  std::unique_ptr<std::map<const wchar_t *, ComObject<FakeFile>, WCharCmp>>
      _children;
  MGDF::IFile *_parent;
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