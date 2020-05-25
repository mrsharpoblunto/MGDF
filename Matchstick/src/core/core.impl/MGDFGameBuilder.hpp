#pragma once

#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

class GameBuilder {
 public:
  static MGDFError LoadGame(
      std::shared_ptr<storage::IStorageFactoryComponent> &storage,
      std::unique_ptr<storage::IGameStorageHandler> &handler,
      ComObject<Game> &game);
};

}  // namespace core
}  // namespace MGDF