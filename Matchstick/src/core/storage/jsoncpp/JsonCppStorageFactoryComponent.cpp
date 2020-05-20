#include "stdafx.h"

#include "JsonCppStorageFactoryComponent.hpp"

#include "JsonCppGameStateStorageHandler.hpp"
#include "JsonCppGameStorageHandler.hpp"
#include "JsonCppPreferenceConfigStorageHandler.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace jsoncppImpl {

IStorageFactoryComponent *CreateJsonCppStorageFactoryComponent() {
  return new JsonCppStorageFactoryComponent();
}

IGameStorageHandler *JsonCppStorageFactoryComponent::CreateGameStorageHandler()
    const {
  return new JsonCppGameStorageHandler();
}

IGameStateStorageHandler *
JsonCppStorageFactoryComponent::CreateGameStateStorageHandler(
    const std::string &game, const Version &version) const {
  return new JsonCppGameStateStorageHandler(game, version);
}

IPreferenceConfigStorageHandler *
JsonCppStorageFactoryComponent::CreatePreferenceConfigStorageHandler() const {
  return new JsonCppPreferenceConfigStorageHandler();
}

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF