#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
#include <unordered_map>
#include <vector>

#include "../storage/MGDFStorageFactoryComponent.hpp"
#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

bool GuidString(std::string &guid);

class GameState;

class PendingSave : public ComBase<IMGDFPendingSave> {
 public:
  PendingSave(ComObject<GameState> &gameState);
  virtual ~PendingSave();
  HRESULT __stdcall GetSaveDataLocation(wchar_t *location, UINT64 *length) final;
  HRESULT Init();

 private:
  std::wstring _saveData;
  ComObject<GameState> _gameState;
  std::string _pendingName;
};

class SaveManager;

class GameState : public ComBase<IMGDFGameState> {
 public:
  GameState(const std::string &saveName, const std::string &gameUid,
            SaveManager *saveManager,
            const std::shared_ptr<storage::IStorageFactoryComponent> &factory);
  GameState(const std::string &gameUid, MGDFVersion &version,
            SaveManager *saveManager,
            const std::shared_ptr<storage::IStorageFactoryComponent> &factory);
  virtual ~GameState() {}
  HRESULT __stdcall GetMetadata(const char *key, char *value, UINT64 *length) final;
  HRESULT __stdcall SetMetadata(const char *key, const char *value) final;
  HRESULT __stdcall GetSaveDataLocation(wchar_t *folder, UINT64 *size) final;
  void __stdcall GetVersion(MGDFVersion *version) final;
  BOOL __stdcall IsNew() final { return _saveName.empty(); }
  HRESULT __stdcall BeginSave(IMGDFPendingSave **pending) final;

  std::string GetSave() const { return _saveName; }
  void SetSave(const std::string &saveName);

  HRESULT Load();
  HRESULT Save();


 private:
  SaveManager *_saves;
  std::unordered_map<std::string, std::string> _metadata;
  std::string _gameUid;
  MGDFVersion _gameVersion;
  std::string _saveName;
  std::shared_ptr<storage::IStorageFactoryComponent> _factory;
};

class SaveManager : public ComBase<IMGDFSaveManager> {
 public:
  SaveManager(
      const ComObject<Game> &game,
      std::shared_ptr<storage::IStorageFactoryComponent> storageFactory);
  ~SaveManager() {}
  UINT64 __stdcall GetSaveCount() final { return _saves.size(); }
  HRESULT __stdcall GetSave(UINT64 index, IMGDFGameState **save) final;
  HRESULT __stdcall DeleteSave(IMGDFGameState *save) final;
  void __stdcall CreateGameState(IMGDFGameState **save) final;

  void AppendSave(const std::string &save) { _saves.push_back(save); }

 private:
  std::shared_ptr<storage::IStorageFactoryComponent> _storageFactory;
  std::string _gameUid;
  MGDFVersion _gameVersion;
  std::vector<std::string> _saves;
};

}  // namespace core
}  // namespace MGDF
