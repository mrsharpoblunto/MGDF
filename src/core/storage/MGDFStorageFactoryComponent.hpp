#pragma once

#include <memory>

#include "MGDFGameStateStorageHandler.hpp"
#include "MGDFGameStorageHandler.hpp"
#include "MGDFPreferenceConfigStorageHandler.hpp"

namespace MGDF {
namespace core {
namespace storage {

class IStorageFactoryComponent {
 public:
  virtual ~IStorageFactoryComponent() {}
  virtual std::unique_ptr<IGameStorageHandler> CreateGameStorageHandler()
      const = 0;
  virtual std::unique_ptr<IGameStateStorageHandler>
  CreateGameStateStorageHandler(const std::string &game,
                                const MGDFVersion &version) const = 0;
  virtual std::unique_ptr<IPreferenceConfigStorageHandler>
  CreatePreferenceConfigStorageHandler() const = 0;
};

}  // namespace storage
}  // namespace core
}  // namespace MGDF