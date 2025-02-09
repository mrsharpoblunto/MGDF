#pragma once

#include "../MGDFGameStorageHandler.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace json {

class JsonGameStorageHandler : public IGameStorageHandler {
 public:
  JsonGameStorageHandler() {
    SecureZeroMemory(&_version, sizeof(MGDFVersion));
  }
  virtual ~JsonGameStorageHandler() {}

  std::string GetGameName() const final;
  std::string GetGameUid() const final;
  std::string GetStatisticsService() const final;
  void GetVersion(MGDFVersion &version) const final;
  const std::map<std::string, std::string> &GetPreferences() const final;

  HRESULT Load(const std::wstring &) final;

 private:
  std::string _gameName, _gameUid, _statisticsService;
  std::map<std::string, std::string> _preferences;
  MGDFVersion _version;
};

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF