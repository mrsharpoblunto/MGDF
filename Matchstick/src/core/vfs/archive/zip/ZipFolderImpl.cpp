#include "StdAfx.h"

#include "ZipFolderImpl.hpp"

#include "../../../common/MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {
namespace zip {

HRESULT ZipFolderImpl::GetLogicalName(wchar_t *name, UINT64 *length) {
  return StringWriter::Write(_name, name, length);
}

}  // namespace zip
}  // namespace vfs
}  // namespace core
}  // namespace MGDF
