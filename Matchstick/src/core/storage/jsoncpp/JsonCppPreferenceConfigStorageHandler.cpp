#include "stdafx.h"

#include "JsonCppPreferenceConfigStorageHandler.hpp"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace jsoncppImpl {

void JsonCppPreferenceConfigStorageHandler::Add(const std::string &name,
                                                const std::string &value) {
  _preferences[name] = value;
}

IPreferenceConfigStorageHandler::iterator
JsonCppPreferenceConfigStorageHandler::begin() const {
  return _preferences.begin();
}

IPreferenceConfigStorageHandler::iterator
JsonCppPreferenceConfigStorageHandler::end() const {
  return _preferences.end();
}

HRESULT JsonCppPreferenceConfigStorageHandler::Load(
    const std::wstring &filename) {
  std::ifstream input(filename.c_str(), std::ios::in);
  if (input.fail()) {
    // TODO handling for other input methods...
    LOG("Unable to open '" << Resources::ToString(filename).c_str() << "'",
        MGDF_LOG_ERROR);
    return E_FAIL;
  }

  Json::Value root;
  Json::Reader reader;

  if (reader.parse(input, root)) {
    for (const auto &name : root.getMemberNames()) {
      if (!root[name].isString()) {
        LOG(reader.getFormatedErrorMessages(), MGDF_LOG_ERROR);
        return E_FAIL;
      }
      _preferences[name] = root[name].asString();
    }
    return S_OK;
  } else {
    LOG("Unable to parse '" << Resources::ToString(filename).c_str() << "'"
                            << reader.getFormatedErrorMessages(),
        MGDF_LOG_ERROR);
    return E_FAIL;
  }
}

void JsonCppPreferenceConfigStorageHandler::Save(
    const std::wstring &filename) const {
  std::ofstream file(filename.c_str(), std::ios::out);

  Json::Value root;

  for (auto &pref : _preferences) {
    root[pref.first] = pref.second;
  }

  file << root;
  file.close();
}

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF