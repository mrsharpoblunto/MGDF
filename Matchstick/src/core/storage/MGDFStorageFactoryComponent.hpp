#pragma once

#include "MGDFGameStateStorageHandler.hpp"
#include "MGDFGameStorageHandler.hpp"
#include "MGDFPreferenceConfigStorageHandler.hpp"

namespace MGDF {
namespace core {
namespace storage {

class IStorageFactoryComponent {
 public:
  virtual ~IStorageFactoryComponent() {}
  virtual IGameStorageHandler *CreateGameStorageHandler() const = 0;
  virtual IGameStateStorageHandler *CreateGameStateStorageHandler(
      const std::string &game, const MGDF::Version &version) const = 0;
  virtual IPreferenceConfigStorageHandler *
  CreatePreferenceConfigStorageHandler() const = 0;
};

}  // namespace storage
}  // namespace core
}  // namespace MGDF