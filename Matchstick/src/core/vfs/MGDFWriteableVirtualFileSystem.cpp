
#include "stdafx.h"

#include "MGDFWriteableVirtualFileSystem.hpp"

#include <MGDF/ComObject.hpp>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFDefaultFileReader.hpp"
#include "MGDFDefaultWriteableFileImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace vfs {

using namespace std::filesystem;

const std::wstring S_DOT(L".");
const std::wstring S_DOTDOT(L"..");

BOOL WriteableVirtualFileSystem::GetFile(const wchar_t *logicalPath,
                                         IMGDFWriteableFile **file) {
  if (!logicalPath) {
    GetRoot(file);
    return true;
  }

  wchar_t *context = 0;
  const size_t destinationLength = wcslen(logicalPath) + 1;
  std::vector<wchar_t> copy(destinationLength);
  wcscpy_s(copy.data(), destinationLength, logicalPath);
  wchar_t *components = wcstok_s(copy.data(), L"/", &context);

  std::filesystem::path path(_rootPath);
  while (components) {
    if (S_DOT != components && S_DOTDOT != components) {
      path /= components;
    }
    components = wcstok_s(0, L"/", &context);
  }

  auto node = MakeCom<DefaultWriteableFileImpl>(
      path.filename().wstring(), path.wstring(), _rootPath.wstring());
  node.AddRawRef(file);
  return true;
}

void WriteableVirtualFileSystem::GetRoot(IMGDFWriteableFile **root) {
    auto r = MakeCom<DefaultWriteableFileImpl>(
        _rootPath.filename().wstring(), _rootPath.wstring(), _rootPath.wstring());
    r.AddRawRef(root);
}

}  // namespace vfs
}  // namespace core
}  // namespace MGDF
