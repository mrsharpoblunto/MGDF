#include "stdafx.h"

#include "JsonStorageFactoryComponent.hpp"

#include "JsonStorageFactoryComponentImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace json {

bool CreateStorageFactoryComponent(
    std::shared_ptr<IStorageFactoryComponent> &comp) {
  comp = std::make_shared<JsonStorageFactoryComponent>();
  return true;
}

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF