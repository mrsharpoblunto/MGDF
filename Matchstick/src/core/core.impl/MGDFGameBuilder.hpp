#pragma once

#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

class GameBuilder {
 public:
  static HRESULT LoadGame(
      std::shared_ptr<storage::IStorageFactoryComponent> &storage,
      const storage::IGameStorageHandler *handler, ComObject<Game> &game);
};

}  // namespace core
}  // namespace MGDF