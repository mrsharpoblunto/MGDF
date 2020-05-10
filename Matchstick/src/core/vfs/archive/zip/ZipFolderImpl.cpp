#include "stdafx.h"

#include "ZipFolderImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

ZipFolderImpl::~ZipFolderImpl() {
  if (!_children) return;
  for (auto child : *_children) {
    delete static_cast<FileBaseImpl *>(child.second);
  }
}

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF