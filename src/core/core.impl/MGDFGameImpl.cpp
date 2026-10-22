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
           const std::string &statisticsService, const MGDFVersion &version,
           std::shared_ptr<storage::IStorageFactoryComponent> &storageFactory)
    : _uid(uid),
      _name(name),
      _version(version),
      _statisticsService(statisticsService),
      _storageFactory(storageFactory) {
  _ASSERTE(storageFactory);
}

BOOL Game::HasPreference(const char *name) {
  if (!name) return false;

  std::lock_guard<std::mutex> lock(_preferencesMutex);
  return _preferences.find(name) != _preferences.end();
}

HRESULT Game::GetPreference(const char *name, char *value, UINT64 *length) {
  if (!name) return E_INVALIDARG;

  std::lock_guard<std::mutex> lock(_preferencesMutex);
  const auto iter = _preferences.find(name);
  if (iter != _preferences.end()) {
    return StringWriter::Write(iter->second, value, length);
  } else {
    return E_NOT_SET;
  }
}

void Game::SetPreference(const char *name, const char *value) {
  if (!name || !value) return;

  std::lock_guard<std::mutex> lock(_preferencesMutex);
  const auto it = _preferences.find(name);
  if (it != _preferences.end()) {
    it->second = value;
  } else {
    _preferences.insert(std::make_pair(name, value));
  }
}

void Game::SetPreferences(IMGDFPreferenceSet *preferences) {
  if (!preferences) return;
  auto prefs = dynamic_cast<PreferenceSetImpl *>(preferences);
  _ASSERTE(prefs);
  std::lock_guard<std::mutex> lock(_preferencesMutex);
  for (auto &p : prefs->Preferences) {
    _preferences[p.first] = p.second;
  }
}

void Game::ResetPreferences() {
  std::wstring filename;
  {
    std::lock_guard<std::mutex> lock(_preferencesMutex);
    filename = _preferencesFile;
  }
  LoadPreferences(filename);
}

void Game::SavePreferences() {
  std::map<std::string, std::string> preferences;
  std::wstring filename;
  {
    std::lock_guard<std::mutex> lock(_preferencesMutex);
    preferences = _preferences;
    filename = _preferencesFile;
  }
  SavePreferenceSnapshot(preferences, filename);
}

void Game::SavePreferenceSnapshot(
    const std::map<std::string, std::string> &preferences,
    const std::wstring &filename) {
  std::unique_ptr<storage::IPreferenceConfigStorageHandler> handler(
      _storageFactory->CreatePreferenceConfigStorageHandler());
  for (auto &pref : preferences) {
    handler->Add(pref.first, pref.second);
  }
  handler->Save(filename);
  LOG("Saved preferences to '" << Resources::ToString(filename)
                               << "' successfully",
      MGDF_LOG_LOW);
}

void Game::SavePreferences(const std::wstring &filename) {
  std::map<std::string, std::string> preferences;
  {
    std::lock_guard<std::mutex> lock(_preferencesMutex);
    _preferencesFile = filename;
    preferences = _preferences;
  }
  SavePreferenceSnapshot(preferences, filename);
}

HRESULT Game::LoadPreferences(const std::wstring &filename) {
  std::unique_ptr<storage::IPreferenceConfigStorageHandler> handler(
      _storageFactory->CreatePreferenceConfigStorageHandler());
  const auto result = handler->Load(filename);
  if (FAILED(result)) {
    return result;
  } else {
    {
      std::lock_guard<std::mutex> lock(_preferencesMutex);
      for (const auto &storedPreference : *handler) {
        _preferences[storedPreference.first] = storedPreference.second;
      }
    }
    LOG("Loaded preferences from '" + Resources::ToString(filename) +
            "' successfully",
        MGDF_LOG_LOW);
    return S_OK;
  }
}

void Game::LoadPreferences(
    const std::map<std::string, std::string> &preferences) {
  std::lock_guard<std::mutex> lock(_preferencesMutex);
  for (auto &pref : preferences) {
    _preferences[pref.first] = pref.second;
  }
}

}  // namespace core
}  // namespace MGDF
