#include "stdafx.h"

#include "JsonCppGameStorageHandler.hpp"

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
namespace jsoncppImpl {

std::string JsonCppGameStorageHandler::GetGameName() const { return _gameName; }

std::string JsonCppGameStorageHandler::GetGameUid() const { return _gameUid; }

INT32 JsonCppGameStorageHandler::GetInterfaceVersion() const {
  return _interfaceVersion;
}

void JsonCppGameStorageHandler::GetVersion(Version &version) const {
  version = _version;
}

const std::map<std::string, std::string>
    &JsonCppGameStorageHandler::GetPreferences() const {
  return _preferences;
}

MGDFError JsonCppGameStorageHandler::Load(const std::wstring &filename) {
  std::ifstream input(filename.c_str(), std::ios::in);
  if (input.fail()) {
    // TODO handling for other input methods...
    LOG("Unable to open '" << Resources::ToString(filename).c_str() << "'",
        LOG_ERROR);
    return MGDF_ERR_INVALID_FILE;
  }

  Json::Value root;
  Json::Reader reader;

  if (reader.parse(input, root)) {
    _gameName = GetJsonValue(root, "gameName");
    _gameUid = GetJsonValue(root, "gameUid");
    _version = VersionHelper::Create(GetJsonValue(root, "version"));
    _interfaceVersion = atoi(GetJsonValue(root, "interfaceVersion").c_str());
    Json::Value preferences = root["preferences"];
    if (!preferences.isNull()) {
      for (const auto &key : preferences.getMemberNames()) {
        _preferences.insert(std::make_pair(key, preferences[key].asString()));
      }
    }
    return MGDF_OK;
  } else {
    LOG("Unable to parse '" << Resources::ToString(filename).c_str() << "'"
                            << reader.getFormatedErrorMessages(),
        LOG_ERROR);
    return MGDF_ERR_INVALID_JSON;
  }
}

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF