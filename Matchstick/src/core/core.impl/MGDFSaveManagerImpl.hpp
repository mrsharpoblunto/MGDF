#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <unordered_map>
#include <vector>

#include "../storage/MGDFStorageFactoryComponent.hpp"
#include "../vfs/MGDFReadOnlyVirtualFileSystemComponentImpl.hpp"
#include "../vfs/MGDFWriteableVirtualFileSystem.hpp"
#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

bool GuidString(std::string &guid);

class GameState;

class PendingSave : public ComBase<IMGDFWriteableVirtualFileSystem> {
 public:
  PendingSave(ComObject<GameState> &gameState);
  virtual ~PendingSave();
  HRESULT Init();

  BOOL __stdcall GetFile(const wchar_t *logicalPath,
                         IMGDFWriteableFile **file) final;
  void __stdcall GetRoot(IMGDFWriteableFile **root) final;
  HRESULT __stdcall GetLogicalPath(IMGDFWriteableFile *file, wchar_t *path,
                                   UINT64 *length) final;

 private:
  ComObject<vfs::WriteableVirtualFileSystem> _vfs;
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
  HRESULT __stdcall GetMetadata(const char *key, char *value,
                                UINT64 *length) final;
  HRESULT __stdcall SetMetadata(const char *key, const char *value) final;
  void __stdcall GetVersion(MGDFVersion *version) final;
  BOOL __stdcall IsNew() final { return _saveName.empty(); }
  HRESULT __stdcall GetVFS(IMGDFReadOnlyVirtualFileSystem **vfs) final;
  HRESULT __stdcall BeginSave(IMGDFWriteableVirtualFileSystem **pending) final;

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
