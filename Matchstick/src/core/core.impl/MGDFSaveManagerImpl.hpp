#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <unordered_map>
#include <vector>

#include "../storage/MGDFStorageFactoryComponent.hpp"
#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

bool GuidString(std::string &guid);

class GameState;

class PendingSave : public ComBase<IPendingSave> {
 public:
  PendingSave(ComObject<GameState> &gameState);
  virtual ~PendingSave();
  HRESULT GetSaveDataLocation(wchar_t *location, size_t *length) const final;
  HRESULT Init();

 private:
  std::wstring _saveData;
  ComObject<GameState> _gameState;
  std::string _pendingName;
};

class SaveManager;

class GameState : public ComBase<IGameState> {
 public:
  GameState(const std::string &saveName, const std::string &gameUid,
            SaveManager *saveManager,
            const std::shared_ptr<storage::IStorageFactoryComponent> &factory);
  GameState(const std::string &gameUid, Version &version,
            SaveManager *saveManager,
            const std::shared_ptr<storage::IStorageFactoryComponent> &factory);
  virtual ~GameState() {}
  HRESULT GetMetadata(const char *key, char *value, size_t *length) const final;
  HRESULT SetMetadata(const char *key, const char *value) final;
  HRESULT GetSaveDataLocation(wchar_t *folder, size_t *size) const final;
  void GetVersion(Version *version) const final;
  bool IsNew() const final { return _saveName.empty(); }

  std::string GetSave() const { return _saveName; }
  void SetSave(const std::string &saveName);

  HRESULT Load();
  HRESULT Save() const;

  HRESULT BeginSave(IPendingSave **pending) final;

 private:
  SaveManager *_saves;
  std::unordered_map<std::string, std::string> _metadata;
  std::string _gameUid;
  Version _gameVersion;
  std::string _saveName;
  std::shared_ptr<storage::IStorageFactoryComponent> _factory;
};

class SaveManager : public ComBase<ISaveManager> {
 public:
  SaveManager(
      const Game *game,
      std::shared_ptr<storage::IStorageFactoryComponent> storageFactory);
  ~SaveManager() {}
  size_t GetSaveCount() const final { return _saves.size(); }
  HRESULT GetSave(size_t index, IGameState **save) final;
  HRESULT DeleteSave(IGameState *save) final;
  void CreateGameState(IGameState **save) final;

  void AppendSave(const std::string &save) { _saves.push_back(save); }

 private:
  std::shared_ptr<storage::IStorageFactoryComponent> _storageFactory;
  std::string _gameUid;
  Version _gameVersion;
  std::vector<std::string> _saves;
};

}  // namespace core
}  // namespace MGDF
