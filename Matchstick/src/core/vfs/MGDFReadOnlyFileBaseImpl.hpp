#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <map>
#include <mutex>
#include <string>

namespace MGDF {
namespace core {
namespace vfs {

struct WStrCmp {
  bool operator()(const std::wstring &a, const std::wstring &b) const {
    return a < b;
  }
};

/**
 abstract class which contains the common functionality to default file
 instances aswell as the zip and other archive file implementations of the
 standard ifile interface
*/
class ReadOnlyFileBaseImpl : public ComBase<IMGDFReadOnlyFile, IMGDFFile> {
 public:
  ReadOnlyFileBaseImpl(IMGDFReadOnlyFile *parent,
                       IMGDFReadOnlyVirtualFileSystem *vfs);
  virtual ~ReadOnlyFileBaseImpl(){};

  BOOL __stdcall GetParent(IMGDFReadOnlyFile **parent) final;
  BOOL __stdcall GetChild(const wchar_t *name,
                          IMGDFReadOnlyFile **child) override;
  UINT64 __stdcall GetChildCount() override;
  void __stdcall GetAllChildren(IMGDFReadOnlyFile **childBuffer) override;
  void __stdcall GetVFS(IMGDFReadOnlyVirtualFileSystem **vfs) final;
  HRESULT __stdcall GetLogicalPath(wchar_t *path, UINT64 *length) final;
  HRESULT __stdcall CopyTo(IMGDFWriteableFile *destination) final;

  // These internal methods are not threadsafe, so ensure
  // that the mutex for this file is acquired or that only
  // one thread can access the file before calling
  void AddChild(const ComObject<IMGDFReadOnlyFile> &newNode);

 protected:
  std::mutex _mutex;
  std::unique_ptr<std::map<std::wstring, ComObject<IMGDFReadOnlyFile>, WStrCmp>>
      _children;
  std::wstring _logicalPath;
  // hold a raw reference here as we don't want children
  // to hold their parent files in scope
  IMGDFReadOnlyFile *_parent;
  IMGDFReadOnlyVirtualFileSystem *_vfs;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF