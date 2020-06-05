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

HRESULT PendingSave::GetSaveDataLocation(wchar_t* location,
                                         size_t* length) const {
  COPY_WSTR(_saveData, location, length);
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
  return S_OK;
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
    LOG("Unable to update save " << _gameState->GetSave(), LOG_ERROR);
  }
}

GameState::GameState(
    const std::string& saveName, const std::string& gameUid, SaveManager* saves,
    const std::shared_ptr<storage::IStorageFactoryComponent>& factory)
    : _saveName(saveName), _gameUid(gameUid), _saves(saves), _factory(factory) {
  ZeroMemory(&_gameVersion, sizeof(Version));
}

GameState::GameState(
    const std::string& gameUid, Version& version, SaveManager* saves,
    const std::shared_ptr<storage::IStorageFactoryComponent>& factory)
    : _gameUid(gameUid),
      _gameVersion(version),
      _saves(saves),
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
  if (handler->Load(saveFile) != MGDF_OK) {
    return E_FAIL;
  }

  handler->GetVersion(_gameVersion);
  handler->GetMetadata(_metadata);
  return S_OK;
}

HRESULT GameState::Save() const {
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

HRESULT GameState::GetMetadata(const char* key, char* value,
                               size_t* length) const {
  auto found = _metadata.find(key);
  if (found == _metadata.end()) {
    return E_FAIL;
  }
  COPY_STR(found->second, value, length);
  return S_OK;
}

HRESULT GameState::SetMetadata(const char* key, const char* value) {
  if (key == nullptr) {
    return E_INVALIDARG;
  }
  _metadata[key] = value;
  return S_OK;
}

HRESULT GameState::GetSaveDataLocation(wchar_t* folder, size_t* size) const {
  if (IsNew()) {
    return E_FAIL;
  }
  auto saveDir = Resources::Instance().SaveDataDir(_saveName);
  COPY_WSTR(saveDir, folder, size);
  return S_OK;
}

void GameState::GetVersion(Version* version) const { *version = _gameVersion; }

HRESULT GameState::BeginSave(IPendingSave** p) {
  auto state = MakeComFromPtr<GameState>(this);
  auto pending = MakeCom<PendingSave>(state);
  if (FAILED(pending->Init())) {
    return E_FAIL;
  }
  pending.AddRawRef(p);
  return S_OK;
}

SaveManager::SaveManager(
    const Game* game,
    std::shared_ptr<storage::IStorageFactoryComponent> storageFactory)
    : _storageFactory(storageFactory) {
  path savePath(Resources::Instance().SaveBaseDir());
  _gameUid = game->GetUid();
  game->GetVersion(&_gameVersion);
  directory_iterator end_itr;  // default construction yields past-the-end
  for (directory_iterator itr(savePath); itr != end_itr; ++itr) {
    if (is_directory(itr->path())) {
      std::wstring saveName(itr->path().filename());
      if (saveName.find(PENDING_SAVE_MARKER) != 0) {
        _saves.push_back(Resources::ToString(saveName));
      }
    }
  }
}

HRESULT SaveManager::GetSave(size_t index, IGameState** s) {
  if (index >= _saves.size()) {
    return E_INVALIDARG;
  }
  auto saveName = _saves.at(index);
  ComObject<GameState> save =
      MakeCom<GameState>(saveName, _gameUid, this, _storageFactory);

  if (FAILED(save->Load())) {
    return E_FAIL;
  }
  save.AddRawRef(s);
  return S_OK;
}

HRESULT SaveManager::DeleteSave(IGameState* s) {
  auto state = MakeComFromPtr<GameState>(s);
  auto found = std::find(_saves.begin(), _saves.end(), state->GetSave());
  if (found == _saves.end()) {
    return E_INVALIDARG;
  }
  try {
    remove_all(Resources::Instance().SaveDir(*found));
    _saves.erase(found);
  } catch (...) {
    LOG("Failed to remove save " << (*found).c_str(), LOG_ERROR);
    return E_FAIL;
  }
  return S_OK;
}

void SaveManager::CreateGameState(IGameState** save) {
  auto state =
      MakeCom<GameState>(_gameUid, _gameVersion, this, _storageFactory);
  state.AddRawRef(save);
}

}  // namespace core
}  // namespace MGDF
