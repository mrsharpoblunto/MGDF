#include "stdafx.h"

#include "JsonPreferenceConfigStorageHandler.hpp"

#include "../../common/MGDFLoggerImpl.hpp"
#include "../../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace storage {
namespace json {

void JsonPreferenceConfigStorageHandler::Add(const std::string &name,
                                             const std::string &value) {
  _preferences[name] = value;
}

IPreferenceConfigStorageHandler::iterator
JsonPreferenceConfigStorageHandler::begin() const {
  return _preferences.begin();
}

IPreferenceConfigStorageHandler::iterator
JsonPreferenceConfigStorageHandler::end() const {
  return _preferences.end();
}

HRESULT JsonPreferenceConfigStorageHandler::Load(const std::wstring &filename) {
  std::ifstream input(filename.c_str(), std::ios::in);
  if (input.fail()) {
    LOG("Unable to open '" << Resources::ToString(filename).c_str() << "'",
        MGDF_LOG_ERROR);
    return E_FAIL;
  }

  try {
    nlohmann::json root;
    input >> root;
    for (const auto &el : root.items()) {
      if (!el.value().is_string()) {
        LOG("Unable to load preference '"
                << el.key() << "' from '"
                << Resources::ToString(filename).c_str()
                << "' as it is not a string",
            MGDF_LOG_ERROR);
        return E_FAIL;
      }
      _preferences[el.key()] = el.value().get<std::string>();
    }
    return S_OK;
  } catch (nlohmann::json::exception &ex) {
    LOG("Unable to parse '" << Resources::ToString(filename).c_str() << "'"
                            << ex.what(),
        MGDF_LOG_ERROR);
    return E_FAIL;
  }
}

void JsonPreferenceConfigStorageHandler::Save(
    const std::wstring &filename) const {
  std::ofstream file(filename.c_str(), std::ios::out);

  nlohmann::json root;
  for (auto &pref : _preferences) {
    root[pref.first] = pref.second;
  }

  file << root;
  file.close();
}

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF