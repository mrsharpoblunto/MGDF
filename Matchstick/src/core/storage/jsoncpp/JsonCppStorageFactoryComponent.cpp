#include "stdafx.h"

#include "JsonCppStorageFactoryComponent.hpp"

#include "JsonCppStorageFactoryComponentImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace jsoncppImpl {

bool CreateStorageFactoryComponent(
    std::shared_ptr<IStorageFactoryComponent> &comp) {
  comp = std::make_shared<JsonCppStorageFactoryComponent>();
  return true;
}

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF