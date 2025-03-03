#include "stdafx.h"

#include "JsonGameStateStorageHandler.hpp"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFResources.hpp"
#include "../../common/MGDFVersionHelper.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace json {

HRESULT JsonGameStateStorageHandler::Load(const std::wstring &filename) {
  std::ifstream input(filename.c_str(), std::ios::in);
  if (input.fail()) {
    LOG("Unable to open '" << Resources::ToString(filename).c_str() << "'",
        MGDF_LOG_ERROR);
    return E_FAIL;
  }
  try {
    nlohmann::json root;
    input >> root;
    _gameUid = GetJsonValue(root, "gameUid");
    _version = VersionHelper::Create(GetJsonValue(root, "gameVersion"));
    _metadata.clear();
    auto metadata = root.find("gameMetadata");
    if (metadata != root.cend()) {
      for (const auto &el : metadata.value().items()) {
        _metadata[el.key()] = el.value().get<std::string>();
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

void JsonGameStateStorageHandler::Save(const std::wstring &filename) const {
  std::ofstream file(filename.c_str(), std::ios::out);

  nlohmann::json root;

  root["gameUid"] = _gameUid;
  root["gameVersion"] = VersionHelper::Format(_version);
  auto &metadata = root["gameMetadata"] = nlohmann::json::object();

  for (const auto &kvp : _metadata) {
    metadata[kvp.first] = kvp.second;
  }

  file << root;
  file.close();
}

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF