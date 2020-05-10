#pragma once

#include <MGDF/MGDF.hpp>

#include "../MGDFGameStateStorageHandler.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace jsoncppImpl {

class JsonCppGameStateStorageHandler : public IGameStateStorageHandler {
 public:
  JsonCppGameStateStorageHandler(const std::string &gameUid,
                                 const Version *version)
      : _gameUid(gameUid), _version(VersionHelper::Copy(version)) {}
  virtual ~JsonCppGameStateStorageHandler() {}

  MGDFError Load(const std::wstring &) override final;
  void Save(const std::wstring &) const override final;

  std::string GetGameUid() const override final { return _gameUid; };
  void SetVersion(const Version *version) override final {
    _version = VersionHelper::Copy(version);
  };
  const Version *GetVersion() const override final { return &_version; };

 private:
  std::string _gameUid;
  Version _version;
};

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF