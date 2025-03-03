#include "stdafx.h"

#include "MGDFVirtualFileSystemComponent.hpp"

#include "MGDFReadOnlyVirtualFileSystemComponent.hpp"
#include "MGDFWriteableVirtualFileSystem.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

bool CreateReadOnlyVirtualFileSystemComponent(
    ComObject<IReadOnlyVirtualFileSystemComponent> &comp) {
  comp = MakeCom<ReadOnlyVirtualFileSystemComponent>();
  return true;
}

bool CreateWriteableVirtualFileSystemComponent(
    const std::wstring &physicalDirectory,
    ComObject<IMGDFWriteableVirtualFileSystem> &comp) {
  comp = MakeCom<WriteableVirtualFileSystem>(physicalDirectory.c_str());
  return true;
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
