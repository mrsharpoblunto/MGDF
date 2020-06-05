#pragma once

#include <MGDF/MGDF.hpp>

#include "../MGDFGameStorageHandler.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace jsoncppImpl {

class JsonCppGameStorageHandler : public IGameStorageHandler {
 public:
  JsonCppGameStorageHandler() { SecureZeroMemory(&_version, sizeof(Version)); }
  virtual ~JsonCppGameStorageHandler() {}

  std::string GetGameName() const final;
  std::string GetGameUid() const final;
  void GetVersion(Version &version) const final;
  const std::map<std::string, std::string> &GetPreferences() const final;

  MGDFError Load(const std::wstring &) final;

 private:
  std::string _gameName, _gameUid;
  std::map<std::string, std::string> _preferences;
  Version _version;
};

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF