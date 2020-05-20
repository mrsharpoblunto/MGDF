#pragma once

#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

class GameBuilder {
 public:
  static MGDFError LoadGame(storage::IStorageFactoryComponent *storage,
                            storage::IGameStorageHandler *handler,
                            ComObject<Game> &game);
};

}  // namespace core
}  // namespace MGDF