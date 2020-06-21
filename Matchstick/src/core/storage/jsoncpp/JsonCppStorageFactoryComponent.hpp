#pragma once

#include <MGDF/MGDF.h>

#include "../MGDFStorageFactoryComponent.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace jsoncppImpl {

class JsonCppStorageFactoryComponent : public IStorageFactoryComponent {
 public:
  JsonCppStorageFactoryComponent() {}
  virtual ~JsonCppStorageFactoryComponent() {}

  std::unique_ptr<IGameStorageHandler> CreateGameStorageHandler() const final;
  std::unique_ptr<IGameStateStorageHandler> CreateGameStateStorageHandler(
      const std::string &, const MGDFVersion &) const final;
  std::unique_ptr<IPreferenceConfigStorageHandler>
  CreatePreferenceConfigStorageHandler() const final;
};

bool CreateJsonCppStorageFactoryComponent(
    std::shared_ptr<IStorageFactoryComponent> &comp);

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF