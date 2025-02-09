#include "stdafx.h"

#include "JsonGameStorageHandler.hpp"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFVersionHelper.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace json {

std::string JsonGameStorageHandler::GetGameName() const { return _gameName; }

std::string JsonGameStorageHandler::GetGameUid() const { return _gameUid; }

std::string JsonGameStorageHandler::GetStatisticsService() const {
  return _statisticsService;
}

void JsonGameStorageHandler::GetVersion(MGDFVersion &version) const {
  version = _version;
}

const std::map<std::string, std::string> &
JsonGameStorageHandler::GetPreferences() const {
  return _preferences;
}

HRESULT JsonGameStorageHandler::Load(const std::wstring &filename) {
  std::ifstream input(filename.c_str(), std::ios::in);
  if (input.fail()) {
    LOG("Unable to open '" << Resources::ToString(filename).c_str() << "'",
        MGDF_LOG_ERROR);
    return E_FAIL;
  }

  try {
    nlohmann::json root;
    input >> root;
    _gameName = GetJsonValue(root, "gameName");
    _gameUid = GetJsonValue(root, "gameUid");
    _statisticsService = GetJsonValue(root, "statisticsService");
    _version = VersionHelper::Create(GetJsonValue(root, "version"));
    auto preferences = root.find("preferences");
    if (preferences != root.end()) {
      for (const auto &el : preferences.value().items()) {
        _preferences.insert(
            std::make_pair(el.key(), el.value().get<std::string>()));
      }
    }
    return S_OK;
  } catch (nlohmann::json::exception &ex) {
    LOG("Unable to parse '" << Resources::ToString(filename).c_str() << "'"
                            << ex.what(),
        MGDF_LOG_ERROR);
    return E_FAIL;
  }
}

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF