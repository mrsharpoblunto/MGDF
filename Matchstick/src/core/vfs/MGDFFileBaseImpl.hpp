#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
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
class FileBaseImpl : public ComBase<IFile> {
 public:
  FileBaseImpl(IFile *parent);
  virtual ~FileBaseImpl(){};

  bool GetParent(IFile **parent) override final;
  bool GetChild(const wchar_t *name, IFile **child) override;
  size_t GetChildCount() override;
  void GetAllChildren(IFile **childBuffer) override;
  bool IsArchive() const override { return false; }
  const wchar_t *GetLogicalPath() override final;
  time_t GetLastWriteTime() const override;

  // These internal methods are not threadsafe, so ensure
  // that the mutex for this file is acquired or that only
  // one thread can access the file before calling
  void AddChild(ComObject<IFile> &newNode);

 protected:
  std::mutex _mutex;
  std::wstring _logicalPath;
  std::unique_ptr<std::map<const wchar_t *, ComObject<IFile>, WCharCmp>>
      _children;
  // hold a raw reference here as we don't want children
  // to hold thier parent files in scope
  IFile *_parent;
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF