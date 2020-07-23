#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <mutex>
#include <string>

#include "../common/MGDFStringImpl.hpp"

namespace MGDF {
namespace core {
namespace vfs {

template <typename T>
class LogicalPathResolver {
 public:
  LogicalPathResolver() : _queryKey(nullptr) {}

  HRESULT GetLogicalPath(T *file, wchar_t *path, UINT64 *length) {
    _ASSERTE(file);
    std::lock_guard<std::mutex> lock(_mutex);

    if (_queryKey != file) {
      // cache the last lookup so that when the caller does the typical
      // double call of this function (to get the size required, then to
      // actually fill the buffer) we don't have to calculate the logical path
      // twice
      _queryKey = file;
      _queryCache.clear();
      if (FAILED(BuildLogicalPath(file, _queryCache))) {
        return E_FAIL;
      }
    }
    auto result = StringWriter::Write(_queryCache, path, length);
    if (SUCCEEDED(result)) {
      _queryKey = nullptr;
    }
    return result;
  }

 private:
  std::mutex _mutex;
  T *_queryKey;
  std::wstring _queryCache;

  HRESULT BuildLogicalPath(T *file, std::wstring &logicalPath) {
    std::vector<ComObject<T>> objectPath;
    ComObject<T> node(file, true);
    do {
      objectPath.push_back(node);
    } while (node->GetParent(node.Assign()));

    for (auto it = objectPath.rbegin() + 1; it != objectPath.rend(); ++it) {
      UINT64 nameLength = 0;
      const UINT64 oldLength = logicalPath.size();
      if (FAILED((*it)->GetLogicalName(nullptr, &nameLength))) {
        return E_FAIL;
      }
      const UINT64 newSize = (oldLength + nameLength) + ((*it != file) ? 1 : 0);
      logicalPath.resize(newSize);
      if (FAILED((*it)->GetLogicalName(logicalPath.data() + oldLength,
                                       &nameLength))) {
        return E_FAIL;
      }
      if ((*it) != file) logicalPath[logicalPath.size() - 1] = L'/';
    }
    return S_OK;
  }
};

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
