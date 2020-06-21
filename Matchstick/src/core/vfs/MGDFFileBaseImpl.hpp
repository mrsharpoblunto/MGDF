#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
#include <map>
#include <mutex>
#include <string>

namespace MGDF {
namespace core {
namespace vfs {

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
class FileBaseImpl : public ComBase<IMGDFFile> {
 public:
  FileBaseImpl(IMGDFFile *parent);
  virtual ~FileBaseImpl(){};

  BOOL __stdcall GetParent(IMGDFFile **parent) final;
  BOOL __stdcall GetChild(const wchar_t *name, IMGDFFile **child) override;
  UINT64 __stdcall GetChildCount() override;
  void __stdcall GetAllChildren(IMGDFFile **childBuffer) override;
  BOOL __stdcall IsArchive() override { return false; }
  const wchar_t * __stdcall GetLogicalPath() final;
  UINT64 __stdcall GetLastWriteTime() override;

  // These internal methods are not threadsafe, so ensure
  // that the mutex for this file is acquired or that only
  // one thread can access the file before calling
  void AddChild(ComObject<IMGDFFile> &newNode);

 protected:
  std::mutex _mutex;
  std::wstring _logicalPath;
  std::unique_ptr<std::map<const wchar_t *, ComObject<IMGDFFile>, WCharCmp>>
      _children;
  // hold a raw reference here as we don't want children
  // to hold thier parent files in scope
  IMGDFFile *_parent;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF