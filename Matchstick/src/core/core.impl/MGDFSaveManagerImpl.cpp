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

bool GuidString(std::string& guid) {
  UUID uuid;
  if (UuidCreate(&uuid) != RPC_S_OK) {
    return false;
  }
  char* uuidStrBuffer;
  if (UuidToStringA(&uuid, (RPC_CSTR*)&uuidStrBuffer) != RPC_S_OK ||
      !uuidStrBuffer) {
    return false;
  }
  guid = std::string(uuidStrBuffer);
  RpcStringFree((RPC_CSTR*)&uuidStrBuffer);
  return true;
}

PendingSave::PendingSave(ComObject<GameState>& gameState)
    : _gameState(gameState) {}

HRESULT PendingSave::Init() {
  if (!GuidString(_pendingName)) {
    return E_FAIL;
  }
  _pendingName.append(Resources::ToString(PENDING_SAVE_MARKER));

  _saveData = Resources::Instance().SaveDir(_pendingName);
  create_directories(_saveData);
  LOG("Mounting pending save directory \'" << Resources::ToString(_saveData)
                                           << "\' into VFS",
      MGDF_LOG_LOW);
  if (!vfs::CreateWriteableVirtualFileSystemComponent(_saveData, _vfs)) {
    return E_FAIL;
  }
  return S_OK;
}

BOOL PendingSave::GetFile(const wchar_t* logicalPath,
                          IMGDFWriteableFile** file) {
  if (!_vfs) {
    return false;
  }
  return _vfs->GetFile(logicalPath, file);
}

void PendingSave::GetRoot(IMGDFWriteableFile** root) {
  if (!_vfs) {
    return;
  }
  _vfs->GetRoot(root);
}

HRESULT PendingSave::GetLogicalPath(IMGDFWriteableFile* file, wchar_t* path,
                                    UINT64* length) {
  if (!_vfs) {
    return E_FAIL;
  }
  return _vfs->GetLogicalPath(file, path, length);
}

PendingSave::~PendingSave() {
  if (_saveData.empty()) {
    // don't do anything if the pending save wasn't initialized correctly
    return;
  }

  if (!_gameState->IsNew()) {
    // swap the pending data over to the existing save state
    std::wstring oldSaveData(
        Resources::Instance().SaveDataDir(_gameState->GetSave()));
    std::wstring oldSaveDataBackup(
        Resources::Instance().SaveDir(_gameState->GetSave()) + L"/backup");
    std::filesystem::rename(oldSaveData, oldSaveDataBackup);
    std::filesystem::rename(_saveData, oldSaveData);
    remove_all(oldSaveDataBackup);
  } else {
    std::string saveName;
    GuidString(saveName);
    _gameState->SetSave(saveName);
    std::wstring newSave(Resources::Instance().SaveDir(_gameState->GetSave()));
    create_directories(newSave);
    std::wstring saveData(
        Resources::Instance().SaveDataDir(_gameState->GetSave()));
    std::filesystem::rename(_saveData, saveData);
  }
  if (FAILED(_gameState->Save())) {
    LOG("Unable to update save " << _gameState->GetSave(), MGDF_LOG_ERROR);
  }
}

GameState::GameState(
    const std::string& saveName, const std::string& gameUid, SaveManager* saves,
    const ComObject<vfs::IReadOnlyVirtualFileSystemComponent>& vfs,
    const std::shared_ptr<storage::IStorageFactoryComponent>& factory)
    : _saveName(saveName),
      _gameUid(gameUid),
      _saves(saves),
      _vfs(vfs),
      _factory(factory) {
  ZeroMemory(&_gameVersion, sizeof(MGDFVersion));
}

GameState::GameState(
    const std::string& gameUid, MGDFVersion& version, SaveManager* saves,
    const ComObject<vfs::IReadOnlyVirtualFileSystemComponent>& vfs,
    const std::shared_ptr<storage::IStorageFactoryComponent>& factory)
    : _gameUid(gameUid),
      _gameVersion(version),
      _saves(saves),
      _vfs(vfs),
      _factory(factory) {}

void GameState::SetSave(const std::string& saveName) {
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

HRESULT GameState::GetMetadata(const char* key, char* value, UINT64* length) {
  const auto found = _metadata.find(key);
  if (found == _metadata.end()) {
    return E_NOT_SET;
  }
  return StringWriter::Write(found->second, value, length);
}

HRESULT GameState::SetMetadata(const char* key, const char* value) {
  if (key == nullptr) {
    return E_INVALIDARG;
  }
  _metadata[key] = value;
  return S_OK;
}

void GameState::GetVersion(MGDFVersion* version) { *version = _gameVersion; }

HRESULT GameState::BeginSave(IMGDFWriteableVirtualFileSystem** p) {
  auto state = MakeComFromPtr<GameState>(this);
  auto pending = MakeCom<PendingSave>(state);
  if (FAILED(pending->Init())) {
    return E_FAIL;
  }
  pending.AddRawRef(p);
  return S_OK;
}

HRESULT GameState::GetVFS(IMGDFReadOnlyVirtualFileSystem** vfs) {
  if (IsNew()) {
    return E_NOT_SET;
  }
  auto saveDir = Resources::Instance().SaveDataDir(_saveName);

  ComObject<vfs::IReadOnlyVirtualFileSystemComponent> vfsImpl;
  if (!vfs::CreateReadOnlyVirtualFileSystemComponent(vfsImpl) ||
      !vfsImpl->Mount(saveDir.c_str())) {
    return E_FAIL;
  }
  vfsImpl.AddRawRef(vfs);
  return S_OK;
}

SaveManager::SaveManager(
    const ComObject<Game>& game,
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

HRESULT SaveManager::GetSave(UINT64 index, IMGDFGameState** s) {
  if (index >= _saves.size()) {
    return E_INVALIDARG;
  }
  auto saveName = _saves.at(index);
  ComObject<GameState> save =
      MakeCom<GameState>(saveName, _gameUid, this, _storageFactory, _vfs);

  if (FAILED(save->Load())) {
    return E_FAIL;
  }
  save.AddRawRef(s);
  return S_OK;
}

HRESULT SaveManager::DeleteSave(IMGDFGameState* s) {
  auto state = MakeComFromPtr<GameState>(s);
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

void SaveManager::CreateGameState(IMGDFGameState** save) {
  auto state =
      MakeCom<GameState>(_gameUid, _gameVersion, this, _storageFactory, _vfs);
  state.AddRawRef(save);
}

}  // namespace core
}  // namespace MGDF
