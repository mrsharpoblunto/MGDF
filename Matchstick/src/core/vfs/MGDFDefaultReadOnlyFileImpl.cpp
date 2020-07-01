#include "StdAfx.h"

#include "MGDFDefaultReadOnlyFileImpl.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

HRESULT DefaultReadOnlyFileImpl::Open(IMGDFFileReader **reader) {
  std::lock_guard<std::mutex> lock(_mutex);

  if (!_reader) {
    auto fileStream = std::make_shared<std::ifstream>(
        _physicalPath.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

    if (fileStream && !fileStream->bad() && fileStream->is_open()) {
      _reader = new DefaultFileReader(fileStream, [this]() {
        const std::lock_guard<std::mutex> lock(_mutex);
        _reader = nullptr;
      });
      *reader = _reader;
      return S_OK;
    } else {
      LOG("Unable to open file stream for "
              << Resources::ToString(_physicalPath) << " - " << GetLastError(),
          MGDF_LOG_ERROR);
      return E_FAIL;
    }
  }
  LOG("File " << Resources::ToString(_physicalPath) << " currently in use",
      MGDF_LOG_ERROR);
  return E_ACCESSDENIED;
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
