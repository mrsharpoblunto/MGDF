#include "StdAfx.h"

#include "MGDFDefaultReadOnlyFileImpl.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

HRESULT DefaultReadOnlyFileImpl::GetPhysicalName(wchar_t *name,
                                                 UINT64 *length) {
  return StringWriter::Write(_name, name, length);
}

HRESULT DefaultReadOnlyFileImpl::GetPhysicalPath(wchar_t *path,
                                                 UINT64 *length) {
  return StringWriter::Write(_path, path, length);
}

HRESULT DefaultReadOnlyFileImpl::GetLogicalName(wchar_t *name, UINT64 *length) {
  return StringWriter::Write(_name, name, length);
}

UINT64 DefaultReadOnlyFileImpl::GetLastWriteTime() {
  struct _stat64 fileInfo;
  if (_wstati64(_path.c_str(), &fileInfo) != 0) {
    LOG("Unable to get last write time for " << Resources::ToString(_path),
        MGDF_LOG_ERROR);
    return 0;
  }
  return fileInfo.st_mtime;
}

HRESULT DefaultReadOnlyFileImpl::Open(IMGDFFileReader **reader) {
  std::lock_guard<std::mutex> lock(_mutex);

  if (!_reader) {
    auto fileStream = std::make_shared<std::ifstream>(
        _path.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if (fileStream && !fileStream->bad() && fileStream->is_open()) {
      ComObject<DefaultReadOnlyFileImpl> self(this, true);
      _reader = new DefaultFileReader(fileStream, [self]() {
        const std::lock_guard<std::mutex> lock(self->_mutex);
        self->_reader = nullptr;
      });
      *reader = _reader;
      return S_OK;
    } else {
      LOG("Unable to open file stream for " << Resources::ToString(_path)
                                            << " - " << GetLastError(),
          MGDF_LOG_ERROR);
      return E_FAIL;
    }
  }
  LOG("File " << Resources::ToString(_path) << " currently in use",
      MGDF_LOG_ERROR);
  return E_ACCESSDENIED;
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
