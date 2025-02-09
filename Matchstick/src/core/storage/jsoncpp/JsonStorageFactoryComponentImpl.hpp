#pragma once

#include <MGDF/MGDF.h>

#include "../MGDFStorageFactoryComponent.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace json {

class JsonStorageFactoryComponent : public IStorageFactoryComponent {
 public:
  JsonStorageFactoryComponent() {}
  virtual ~JsonStorageFactoryComponent() {}

  std::unique_ptr<IGameStorageHandler> CreateGameStorageHandler() const final;
  std::unique_ptr<IGameStateStorageHandler> CreateGameStateStorageHandler(
      const std::string &, const MGDFVersion &) const final;
  std::unique_ptr<IPreferenceConfigStorageHandler>
  CreatePreferenceConfigStorageHandler() const final;
};

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF
