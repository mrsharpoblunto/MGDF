#include "stdafx.h"

#include <functional>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/core.impl/MGDFGameImpl.hpp"

using namespace MGDF;
using namespace MGDF::core;
using namespace MGDF::core::storage;

namespace {

struct PreferenceStorage {
  std::function<void()> OnAdd;
  std::map<std::string, std::string> Saved;
};

class PreferenceHandler : public IPreferenceConfigStorageHandler {
 public:
  explicit PreferenceHandler(PreferenceStorage &storage) : _storage(storage) {}

  void Add(const std::string &name, const std::string &value) final {
    _preferences[name] = value;
    if (_storage.OnAdd) {
      auto onAdd = std::move(_storage.OnAdd);
      onAdd();
    }
  }
  iterator begin() const final { return _preferences.begin(); }
  iterator end() const final { return _preferences.end(); }
  HRESULT Load(const std::wstring &) final { return E_NOTIMPL; }
  void Save(const std::wstring &) const final { _storage.Saved = _preferences; }

 private:
  PreferenceStorage &_storage;
  std::map<std::string, std::string> _preferences;
};

class PreferenceFactory : public IStorageFactoryComponent {
 public:
  explicit PreferenceFactory(PreferenceStorage &storage) : _storage(storage) {}

  std::unique_ptr<IGameStorageHandler> CreateGameStorageHandler() const final {
    return nullptr;
  }
  std::unique_ptr<IGameStateStorageHandler> CreateGameStateStorageHandler(
      const std::string &, const MGDFVersion &) const final {
    return nullptr;
  }
  std::unique_ptr<IPreferenceConfigStorageHandler>
  CreatePreferenceConfigStorageHandler() const final {
    return std::make_unique<PreferenceHandler>(_storage);
  }

 private:
  PreferenceStorage &_storage;
};

}  // namespace

TEST(GamePreferencesTests, SaveUsesSnapshotWhenPreferencesChangeDuringStorage) {
  Resources::Instance(GetModuleHandleW(L"core.tests.exe"));
  PreferenceStorage storage;
  std::shared_ptr<IStorageFactoryComponent> factory =
      std::make_shared<PreferenceFactory>(storage);
  auto game =
      MakeCom<Game>("preferences", "Preferences", "", MGDFVersion{}, factory);
  game->SetPreference("a", "first");
  game->SetPreference("z", "before");
  storage.OnAdd = [&]() {
    game->SetPreference("a", "changed-first");
    game->SetPreference("z", "after");
  };

  game->SavePreferences();

  EXPECT_EQ("first", storage.Saved.at("a"));
  EXPECT_EQ("before", storage.Saved.at("z"));
  EXPECT_EQ("after",
            std::string(ComString<&IMGDFGame::GetPreference>(game, "z")));
  game->SavePreferences();
  EXPECT_EQ("changed-first", storage.Saved.at("a"));
  EXPECT_EQ("after", storage.Saved.at("z"));
}
