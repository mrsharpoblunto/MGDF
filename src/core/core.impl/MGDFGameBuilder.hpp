#pragma once

#include "../storage/MGDFStorageFactoryComponent.hpp"
#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

class GameBuilder {
 public:
  static HRESULT LoadGame(
      std::shared_ptr<storage::IStorageFactoryComponent> &storage,
      const std::unique_ptr<storage::IGameStorageHandler> &handler,
      ComObject<Game> &game);
};

}  // namespace core
}  // namespace MGDF