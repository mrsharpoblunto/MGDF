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
                                 const Version &version)
      : _gameUid(gameUid), _version(version) {}
  virtual ~JsonCppGameStateStorageHandler() {}

  MGDFError Load(const std::wstring &) override final;
  void Save(const std::wstring &) const override final;

  std::string GetGameUid() const override final { return _gameUid; };
  void SetVersion(const Version &version) override final {
    _version = version;
  };
  void GetVersion(Version &version) const override final {
    version = _version;
  };

  void GetMetadata(std::unordered_map<std::string, std::string> &metadata)
      const override final {
    metadata = _metadata;
  }
  void SetMetadata(const std::unordered_map<std::string, std::string> &metadata)
      override final {
    _metadata = metadata;
  }

 private:
  std::string _gameUid;
  std::unordered_map<std::string, std::string> _metadata;
  Version _version;
};

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF