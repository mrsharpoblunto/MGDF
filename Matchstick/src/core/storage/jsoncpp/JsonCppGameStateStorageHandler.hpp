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

  MGDFError Load(const std::wstring &) final;
  void Save(const std::wstring &) const final;

  std::string GetGameUid() const final { return _gameUid; };
  void SetVersion(const Version &version) final { _version = version; };
  void GetVersion(Version &version) const final { version = _version; };

  void GetMetadata(
      std::unordered_map<std::string, std::string> &metadata) const final {
    metadata = _metadata;
  }
  void SetMetadata(
      const std::unordered_map<std::string, std::string> &metadata) final {
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