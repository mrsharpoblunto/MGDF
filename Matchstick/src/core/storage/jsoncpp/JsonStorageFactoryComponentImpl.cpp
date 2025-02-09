#include "stdafx.h"

#include "JsonStorageFactoryComponentImpl.hpp"

#include "JsonGameStateStorageHandler.hpp"
#include "JsonGameStorageHandler.hpp"
#include "JsonPreferenceConfigStorageHandler.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace json {

std::unique_ptr<IGameStorageHandler>
JsonStorageFactoryComponent::CreateGameStorageHandler() const {
  return std::make_unique<JsonGameStorageHandler>();
}

std::unique_ptr<IGameStateStorageHandler>
JsonStorageFactoryComponent::CreateGameStateStorageHandler(
    const std::string &game, const MGDFVersion &version) const {
  return std::make_unique<JsonGameStateStorageHandler>(game, version);
}

std::unique_ptr<IPreferenceConfigStorageHandler>
JsonStorageFactoryComponent::CreatePreferenceConfigStorageHandler() const {
  return std::make_unique<JsonPreferenceConfigStorageHandler>();
}

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF