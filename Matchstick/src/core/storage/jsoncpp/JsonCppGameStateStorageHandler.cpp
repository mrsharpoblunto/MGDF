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

MGDFError JsonCppGameStateStorageHandler::Load(const std::wstring &filename) {
  std::ifstream input(filename.c_str(), std::ios::in);

  Json::Value root;
  Json::Reader reader;

  if (reader.parse(input, root)) {
    _gameUid = GetJsonValue(root, "gameUid");
    _version = VersionHelper::Create(GetJsonValue(root, "gameVersion"));
    return MGDF_OK;
  } else {
    LOG(reader.getFormatedErrorMessages(), LOG_ERROR);
    return MGDF_ERR_INVALID_JSON;
  }
}

void JsonCppGameStateStorageHandler::Save(const std::wstring &filename) const {
  std::ofstream file(filename.c_str(), std::ios::out);

  Json::Value root;

  root["gameUid"] = _gameUid;
  root["gameVersion"] = VersionHelper::Format(&_version);

  file << root;
  file.close();
}

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF