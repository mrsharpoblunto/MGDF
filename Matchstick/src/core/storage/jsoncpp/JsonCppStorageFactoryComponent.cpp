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

bool CreateJsonCppStorageFactoryComponent(
    std::shared_ptr<IStorageFactoryComponent> &comp) {
  comp = std::make_shared<JsonCppStorageFactoryComponent>();
  return true;
}

std::unique_ptr<IGameStorageHandler>
JsonCppStorageFactoryComponent::CreateGameStorageHandler() const {
  return std::make_unique<JsonCppGameStorageHandler>();
}

std::unique_ptr<IGameStateStorageHandler>
JsonCppStorageFactoryComponent::CreateGameStateStorageHandler(
    const std::string &game, const MGDFVersion &version) const {
  return std::make_unique<JsonCppGameStateStorageHandler>(game, version);
}

std::unique_ptr<IPreferenceConfigStorageHandler>
JsonCppStorageFactoryComponent::CreatePreferenceConfigStorageHandler() const {
  return std::make_unique<JsonCppPreferenceConfigStorageHandler>();
}

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF