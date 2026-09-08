#include "stdafx.h"

#include "MGDFSaveManagerImpl.hpp"

#include <filesystem>

#include "../common/MGDFResources.hpp"
#include "../common/MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

#define PENDING_SAVE_MARKER L"__"
#define PENDING_SAVE_BACKUP "_backup"

using namespace std::filesystem;

namespace MGDF {
namespace core {

bool GuidString(std::string &guid) {
  UUID uuid;
  if (UuidCreate(&uuid) != RPC_S_OK) {
    return false;
  }
  char *uuidStrBuffer;
  if (UuidToStringA(&uuid, (RPC_CSTR *)&uuidStrBuffer) != RPC_S_OK ||
      !uuidStrBuffer) {
    return false;
  }
  guid = std::string(uuidStrBuffer);
  RpcStringFree((RPC_CSTR *)&uuidStrBuffer);
  return true;
}

PendingSave::PendingSave(ComObject<GameState> &gameState)
    : _gameState(gameState) {}

HRESULT PendingSave::Init() {
  if (!GuidString(_pendingName)) {
    return E_FAIL;
  }
  _pendingName.append(Resources::ToString(PENDING_SAVE_MARKER));

  _saveData =
      (path(Resources::Instance().SaveBaseDir()) / _pendingName).wstring();
  std::error_code error;
  create_directories(_saveData, error);
  if (error) {
    return HRESULT_FROM_WIN32(error.value());
  }
  LOG("Mounting pending save directory \'" << Resources::ToString(_saveData)
                                           << "\' into VFS",
      MGDF_LOG_LOW);
  if (!vfs::CreateWriteableVirtualFileSystemComponent(_saveData, _vfs)) {
    return E_FAIL;
  }
  _initialized = true;
  return S_OK;
}

BOOL PendingSave::GetFile(const wchar_t *logicalPath,
                          IMGDFWriteableFile **file) {
  if (!_vfs) {
    return false;
  }
  return _vfs->GetFile(logicalPath, file);
}

void PendingSave::GetRoot(IMGDFWriteableFile **root) {
  if (!_vfs) {
    return;
  }
  _vfs->GetRoot(root);
}

PendingSave::~PendingSave() {
  if (!_initialized) return;

  _vfs.Clear();
  std::error_code error;
  const bool isNew = _gameState->IsNew();
  std::string saveName = _gameState->GetSave();
  if (isNew && !GuidString(saveName)) {
    LOG("Unable to allocate a save name", MGDF_LOG_ERROR);
    return;
  }
  const path saveDir(Resources::Instance().SaveDir(saveName));
  const path saveData = saveDir / L"data";
  const path backup = saveDir / L"backup";
  if (isNew) {
    create_directories(saveDir, error);
  } else {
    remove_all(backup, error);
    if (!error) rename(saveData, backup, error);
  }
  if (error) {
    LOG("Unable to prepare save " << saveName << ": " << error.message(),
        MGDF_LOG_ERROR);
    return;
  }

  rename(_saveData, saveData, error);
  if (error) {
    LOG("Unable to commit save " << saveName << ": " << error.message(),
        MGDF_LOG_ERROR);
    if (!isNew) {
      std::error_code restoreError;
      rename(backup, saveData, restoreError);
      if (restoreError) {
        LOG("Unable to restore save " << saveName << ": "
                                      << restoreError.message(),
            MGDF_LOG_ERROR);
      }
    }
    return;
  }
  if (isNew) _gameState->SetSave(saveName);
  if (FAILED(_gameState->Save())) {
    LOG("Unable to update save " << saveName, MGDF_LOG_ERROR);
    return;
  }
  remove_all(backup, error);
  if (error) {
    LOG("Unable to remove save backup " << saveName << ": " << error.message(),
        MGDF_LOG_ERROR);
  }
}

GameState::GameState(
    const std::string &saveName, const std::string &gameUid, SaveManager *saves,
    const ComObject<vfs::IReadOnlyVirtualFileSystemComponent> &vfs,
    const std::shared_ptr<storage::IStorageFactoryComponent> &factory)
    : _saveName(saveName),
      _gameUid(gameUid),
      _saves(saves),
      _vfs(vfs),
      _factory(factory) {
  ZeroMemory(&_gameVersion, sizeof(MGDFVersion));
}

GameState::GameState(
    const std::string &gameUid, MGDFVersion &version, SaveManager *saves,
    const ComObject<vfs::IReadOnlyVirtualFileSystemComponent> &vfs,
    const std::shared_ptr<storage::IStorageFactoryComponent> &factory)
    : _gameUid(gameUid),
      _gameVersion(version),
      _saves(saves),
      _vfs(vfs),
      _factory(factory) {}

void GameState::SetSave(const std::string &saveName) {
  _saveName = saveName;
  _saves->AppendSave(saveName);
}

HRESULT GameState::Load() {
  if (IsNew()) {
    return E_FAIL;
  }
  auto handler =
      _factory->CreateGameStateStorageHandler(_gameUid, _gameVersion);

  auto saveFile = Resources::Instance().GameStateSaveFile(_saveName);
  const auto result = handler->Load(saveFile);
  if (SUCCEEDED(result)) {
    handler->GetVersion(_gameVersion);
    handler->GetMetadata(_metadata);
  }
  return result;
}

HRESULT GameState::Save() {
  if (IsNew()) {
    return E_FAIL;
  }
  auto handler =
      _factory->CreateGameStateStorageHandler(_gameUid, _gameVersion);

  handler->SetVersion(_gameVersion);
  handler->SetMetadata(_metadata);
  auto saveFile = Resources::Instance().GameStateSaveFile(_saveName);
  handler->Save(saveFile);
  return S_OK;
}

HRESULT GameState::GetMetadata(const char *key, char *value, UINT64 *length) {
  const auto found = _metadata.find(key);
  if (found == _metadata.end()) {
    return E_NOT_SET;
  }
  return StringWriter::Write(found->second, value, length);
}

HRESULT GameState::SetMetadata(const char *key, const char *value) {
  if (key == nullptr) {
    return E_INVALIDARG;
  }
  _metadata[key] = value;
  return S_OK;
}

void GameState::GetVersion(MGDFVersion *version) { *version = _gameVersion; }

HRESULT GameState::BeginSave(IMGDFWriteableVirtualFileSystem **p,
                             IMGDFReadOnlyVirtualFileSystem **previous) {
  if (!p) return E_POINTER;
  *p = nullptr;
  if (previous) *previous = nullptr;

  ComObject<IMGDFReadOnlyVirtualFileSystem> previousVFS;
  if (!IsNew()) {
    const HRESULT result = GetVFS(previousVFS.Assign());
    if (FAILED(result)) return result;
  }

  auto state = MakeComFromPtr<GameState>(this);
  auto pending = MakeCom<PendingSave>(state);
  const HRESULT result = pending->Init();
  if (FAILED(result)) return result;
  if (previous && previousVFS) previousVFS.AddRawRef(previous);
  pending.AddRawRef(p);
  return S_OK;
}

HRESULT GameState::GetVFS(IMGDFReadOnlyVirtualFileSystem **vfs) {
  if (!vfs) return E_POINTER;
  *vfs = nullptr;
  if (IsNew()) return E_NOT_SET;
  const path saveRoot(Resources::Instance().SaveDir(_saveName));
  const path saveDir = saveRoot / L"data";
  const path backup = saveRoot / L"backup";
  std::error_code error;
  const bool hasData = exists(saveDir, error);
  if (error) return HRESULT_FROM_WIN32(error.value());
  if (!hasData) {
    const bool hasBackup = is_directory(backup, error);
    if (error) return HRESULT_FROM_WIN32(error.value());
    if (!hasBackup) return E_FAIL;
    rename(backup, saveDir, error);
    if (error) return HRESULT_FROM_WIN32(error.value());
  }

  ComObject<vfs::IReadOnlyVirtualFileSystemComponent> vfsImpl;
  if (!vfs::CreateReadOnlyVirtualFileSystemComponent(vfsImpl) ||
      !vfsImpl->Mount(saveDir.c_str())) {
    return E_FAIL;
  }
  vfsImpl.AddRawRef(vfs);
  return S_OK;
}

SaveManager::SaveManager(
    const ComObject<Game> &game,
    ComObject<vfs::IReadOnlyVirtualFileSystemComponent> vfs,
    std::shared_ptr<storage::IStorageFactoryComponent> storageFactory)
    : _vfs(vfs), _storageFactory(storageFactory) {
  path savePath(Resources::Instance().SaveBaseDir());
  _gameUid = game->GetUid();
  game->GetVersion(&_gameVersion);
  directory_iterator end_itr;  // default construction yields past-the-end
  for (directory_iterator itr(savePath); itr != end_itr; ++itr) {
    if (is_directory(itr->path())) {
      std::wstring saveName(itr->path().filename());
      if (saveName.find(PENDING_SAVE_MARKER) == std::wstring::npos) {
        _saves.push_back(Resources::ToString(saveName));
      }
    }
  }
}

HRESULT SaveManager::GetSave(UINT64 index, IMGDFGameState **s) {
  if (index >= _saves.size()) {
    return E_INVALIDARG;
  }
  auto saveName = _saves.at(index);
  ComObject<GameState> save =
      MakeCom<GameState>(saveName, _gameUid, this, _vfs, _storageFactory);

  if (FAILED(save->Load())) {
    return E_FAIL;
  }
  save.AddRawRef(s);
  return S_OK;
}

HRESULT SaveManager::DeleteSave(IMGDFGameState *s) {
  auto state = dynamic_cast<GameState *>(s);
  _ASSERTE(state);
  auto found = std::find(_saves.begin(), _saves.end(), state->GetSave());
  if (found == _saves.end()) {
    return E_INVALIDARG;
  }

  std::error_code code;
  remove_all(Resources::Instance().SaveDir(*found), code);
  if (code.value()) {
    LOG("Failed to remove save " << (*found).c_str(), MGDF_LOG_ERROR);
    return E_FAIL;
  } else {
    _saves.erase(found);
    return S_OK;
  }
}

void SaveManager::CreateGameState(IMGDFGameState **save) {
  auto state =
      MakeCom<GameState>(_gameUid, _gameVersion, this, _vfs, _storageFactory);
  state.AddRawRef(save);
}

}  // namespace core
}  // namespace MGDF
