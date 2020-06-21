#include "stdafx.h"

#include "JsonCppGameStateStorageHandler.hpp"

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

HRESULT JsonCppGameStateStorageHandler::Load(const std::wstring &filename) {
  std::ifstream input(filename.c_str(), std::ios::in);

  Json::Value root;
  Json::Reader reader;

  if (input.fail()) {
    // TODO handling for other input methods...
    LOG("Unable to open '" << Resources::ToString(filename).c_str() << "'",
        MGDF_LOG_ERROR);
    return E_FAIL;
  }
  if (reader.parse(input, root)) {
    _gameUid = GetJsonValue(root, "gameUid");
    _version = VersionHelper::Create(GetJsonValue(root, "gameVersion"));
    _metadata.clear();
    if (root.isMember("gameMetadata")) {
      auto meta = root["gameMetadata"];
      auto keys = meta.getMemberNames();
      for (const auto &key : keys) {
        _metadata[key] = GetJsonValue(meta, key);
      }
    }
    return S_OK;
  } else {
    LOG("Unable to parse '" << Resources::ToString(filename).c_str() << "'"
                            << reader.getFormatedErrorMessages(),
        MGDF_LOG_ERROR);
    return E_FAIL;
  }
}

void JsonCppGameStateStorageHandler::Save(const std::wstring &filename) const {
  std::ofstream file(filename.c_str(), std::ios::out);

  Json::Value root;

  root["gameUid"] = _gameUid;
  root["gameVersion"] = VersionHelper::Format(_version);
  root["gameMetadata"] = Json::Value(Json::objectValue);

  for (const auto &kvp : _metadata) {
    root["gameMetadata"][kvp.first] = kvp.second;
  }

  file << root;
  file.close();
}

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF