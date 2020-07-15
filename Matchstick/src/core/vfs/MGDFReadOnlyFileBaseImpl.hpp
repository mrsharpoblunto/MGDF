#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
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

typedef struct ChildFileRef {
  ComObject<IMGDFReadOnlyFile> Ref;
  std::wstring Name;
} ChildFileRef;

/**
 abstract class which contains the common functionality to default file
 instances aswell as the zip and other archive file implementations of the
 standard ifile interface
*/
class ReadOnlyFileBaseImpl : public ComBase<IMGDFReadOnlyFile> {
 public:
  ReadOnlyFileBaseImpl(IMGDFReadOnlyFile *parent);
  virtual ~ReadOnlyFileBaseImpl(){};

  BOOL __stdcall GetParent(IMGDFReadOnlyFile **parent) final;
  BOOL __stdcall GetChild(const wchar_t *name,
                          IMGDFReadOnlyFile **child) override;
  UINT64 __stdcall GetChildCount() override;
  void __stdcall GetAllChildren(IMGDFReadOnlyFile **childBuffer) override;

  // These internal methods are not threadsafe, so ensure
  // that the mutex for this file is acquired or that only
  // one thread can access the file before calling
  void AddChild(const ComObject<IMGDFReadOnlyFile> &newNode);

 protected:
  std::mutex _mutex;
  std::unique_ptr<std::map<const wchar_t *, ChildFileRef, WCharCmp>> _children;
  std::wstring _logicalPath;
  // hold a raw reference here as we don't want children
  // to hold their parent files in scope
  IMGDFReadOnlyFile *_parent;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF