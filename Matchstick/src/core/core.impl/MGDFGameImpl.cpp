#include "StdAfx.h"

#include "MGDFGameImpl.hpp"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFVersionHelper.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

Game::Game(const std::string &uid, const std::string &name,
           const Version &version,
           std::shared_ptr<storage::IStorageFactoryComponent> &storageFactory)
    : _uid(uid),
      _name(name),
      _version(version),
      _storageFactory(storageFactory) {
  _ASSERTE(storageFactory);
}

bool Game::HasPreference(const char *name) const {
  if (!name) return false;

  return _preferences.find(name) != _preferences.end();
}

HRESULT Game::GetPreference(const char *name, char *value, size_t *length) {
  if (!name) return E_FAIL;

  auto iter = _preferences.find(name);
  if (iter != _preferences.end()) {
    COPY_STR(iter->second, value, length);
  } else {
    return E_FAIL;
  }
}

void Game::SetPreference(const char *name, const char *value) {
  if (!name || !value) return;

  auto it = _preferences.find(name);
  if (it != _preferences.end()) {
    it->second = value;
  } else {
    _preferences.insert(std::make_pair(name, value));
  }
}

void Game::SetPreferences(IPreferenceSet *preferences) {
  if (!preferences) return;
  auto prefs = dynamic_cast<PreferenceSetImpl *>(preferences);
  _ASSERTE(prefs);
  for (auto &p : prefs->Preferences) {
    SetPreference(p.first.c_str(), p.second.c_str());
  }
}

void Game::ResetPreferences() { LoadPreferences(_preferencesFile.c_str()); }

void Game::SavePreferences() const {
  std::unique_ptr<storage::IPreferenceConfigStorageHandler> handler(
      _storageFactory->CreatePreferenceConfigStorageHandler());
  for (auto &pref : _preferences) {
    handler->Add(pref.first, pref.second);
  }
  handler->Save(_preferencesFile);
  LOG("Saved preferences to '" << Resources::ToString(_preferencesFile)
                               << "' successfully",
      LOG_LOW);
}

void Game::SavePreferences(const std::wstring &filename) {
  _preferencesFile = filename;
  SavePreferences();
}

MGDFError Game::LoadPreferences(const std::wstring &filename) {
  std::unique_ptr<storage::IPreferenceConfigStorageHandler> handler(
      _storageFactory->CreatePreferenceConfigStorageHandler());
  const MGDFError result = handler->Load(filename);
  if (MGDF_OK != result) {
    return result;
  } else {
    for (auto storedPreference : *handler) {
      _preferences[storedPreference.first] = storedPreference.second;
    }
    LOG("Loaded preferences from '" + Resources::ToString(filename) +
            "' successfully",
        LOG_LOW);
    return MGDF_OK;
  }
}

void Game::LoadPreferences(
    const std::map<std::string, std::string> &preferences) {
  for (auto &pref : preferences) {
    _preferences[pref.first] = pref.second;
  }
}

}  // namespace core
}  // namespace MGDF