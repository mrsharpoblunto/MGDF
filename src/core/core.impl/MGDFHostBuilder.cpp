#include "StdAfx.h"

#include "MGDFHostBuilder.hpp"

#include <MGDF/MGDF.h>

#include <filesystem>
#include <fstream>

#include "../audio/openal/OpenALSoundManagerComponent.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../common/MGDFResources.hpp"
#include "../input/xinput/XInputManagerComponent.hpp"
#include "../network/mongoose/MongooseNetworkManagerComponent.hpp"
#include "../storage/jsoncpp/JsonStorageFactoryComponent.hpp"
#include "MGDFGameBuilder.hpp"
#include "MGDFParameterConstants.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::filesystem;

namespace MGDF {
namespace core {

bool HostBuilder::RegisterBaseComponents(HostComponents &components) {
  // init global common components
  InitParameterManager();
  InitResources();

  const network::NetworkManagerOptions options{
      .HttpClientOriginConnectionLimit = 4U,
      .HttpClientConnectionTimeout = 10000,
      .HttpClientKeepAlive = 10000,
      .WebSocketClientReconnectInterval = 500};
  if (!network::mongoose::CreateNetworkManagerComponent(components.Network,
                                                        options)) {
    LOG("FATAL ERROR: Unable to register NetworkManager", MGDF_LOG_ERROR);
    return false;
  }

  InitLogger(components.Network);

  if (!storage::json::CreateStorageFactoryComponent(components.Storage)) {
    LOG("FATAL ERROR: Unable to register StorageFactory", MGDF_LOG_ERROR);
    return false;
  }

  return true;
}

bool HostBuilder::RegisterAdditionalComponents(std::string gameUid,
                                               HostComponents &components) {
  if (!input::xinput::CreateInputManagerComponent(components.Input)) {
    LOG("FATAL ERROR: Unable to register InputManager", MGDF_LOG_ERROR);
    return false;
  }

  if (!vfs::CreateReadOnlyVirtualFileSystemComponent(components.VFS)) {
    LOG("FATAL ERROR: Unable to register VirtualFileSystem", MGDF_LOG_ERROR);
    return false;
  }

  if (!audio::openal_audio::CreateSoundManagerComponent(
          components.VFS.As<IMGDFReadOnlyVirtualFileSystem>(),
          components.Sound)) {
    // its a problem, but we can still probably run if the soundmanager failed
    // to initialize
    LOG("ERROR: Unable to register SoundManager", MGDF_LOG_ERROR);
  }

  return true;
}

HRESULT HostBuilder::TryCreateHost(ComObject<Host> &host) {
  HostComponents components;
  memset(&components, 0, sizeof(HostComponents));

  // do the bare minimum setup required to be able to
  // load up the game configuration file
  if (!RegisterBaseComponents(components)) {
    return E_FAIL;
  }

  // try and load the game configuration file
  _ASSERTE(components.Storage);

  auto handler(components.Storage->CreateGameStorageHandler());
  if (!handler) {
    return E_FAIL;
  }

  auto result = handler->Load(Resources::Instance().GameFile());
  if (FAILED(result)) {
    return result;
  }

  // now that we know the UID for the game, we'll set up the user resources
  // paths again and shift the logfile over to the new user directory
  InitResources(handler->GetGameUid());
  Logger::Instance().MoveOutputFile();

  ComObject<Game> game;
  result = GameBuilder::LoadGame(components.Storage, handler, game);
  if (FAILED(result)) {
    LOG("FATAL ERROR: Unable to load game configuration", MGDF_LOG_ERROR);
    return result;
  }

  // now that the game file loaded, initialize everything else
  // and set the log directory correctly.
  if (!RegisterAdditionalComponents(game->GetUid(), components)) {
    return E_FAIL;
  }

  LOG("Creating host...", MGDF_LOG_LOW);
  result = Host::TryCreate(game, components, host);
  if (FAILED(result)) {
    LOG("FATAL ERROR: Unable to create host", MGDF_LOG_ERROR);
    return result;
  }

  return S_OK;
}

void HostBuilder::InitParameterManager() {
  std::string paramString;

  // parse the command line
  std::string cmdLine = GetCommandLine();
  auto cmdLineIter = cmdLine.begin();

  // Skip past program name (first token in command line).
  if (*cmdLineIter == '"') {  // Check for and handle quoted program name
    cmdLineIter++;

    // Skip over until another double-quote or a nullptr
    while (cmdLineIter != cmdLine.end() && *cmdLineIter != '"') ++cmdLineIter;

    // Skip over double-quote
    if (*cmdLineIter == '"') ++cmdLineIter;
  } else {
    // First token wasn't a quote
    while (*cmdLineIter > ' ') ++cmdLineIter;
  }
  paramString = std::string(cmdLineIter, cmdLine.end());
  ParameterManager::Instance().AddParameterString(paramString.c_str());

  // then override with the supplied params.txt in the application path (if provided)
  // providing a params.txt can be useful for debugging purposes.
  path paramsTxt(Resources::Instance().ParamsFile());
  if (exists(paramsTxt)) {
    std::ifstream input(paramsTxt.wstring().c_str(), std::ios::in);
    std::stringstream buffer;
    buffer << input.rdbuf();
    paramString = buffer.str();
    if (paramString.starts_with("//")) {
      paramString.clear();
    }
    ParameterManager::Instance().AddParameterString(paramString.c_str());
  }
}

void HostBuilder::InitLogger(
    const std::shared_ptr<network::INetworkManagerComponent> &network) {
  if (ParameterManager::Instance().HasParameter(
          ParameterConstants::LOG_LEVEL)) {
    const char *level = ParameterManager::Instance().GetParameter(
        ParameterConstants::LOG_LEVEL);

    if (level == ParameterConstants::VALUE_LOG_LEVEL_ERROR) {
      Logger::Instance().SetLoggingLevel(MGDF_LOG_ERROR);
    }
    if (level == ParameterConstants::VALUE_LOG_LEVEL_LOW) {
      Logger::Instance().SetLoggingLevel(MGDF_LOG_LOW);
    }
    if (level == ParameterConstants::VALUE_LOG_LEVEL_MEDIUM) {
      Logger::Instance().SetLoggingLevel(MGDF_LOG_MEDIUM);
    }
    if (level == ParameterConstants::VALUE_LOG_LEVEL_HIGH) {
      Logger::Instance().SetLoggingLevel(MGDF_LOG_HIGH);
    }
  }
  if (ParameterManager::Instance().HasParameter(
          ParameterConstants::LOG_ENDPOINT)) {
    const std::string endpoint(ParameterManager::Instance().GetParameter(
        ParameterConstants::LOG_ENDPOINT));
    Logger::Instance().SetRemoteEndpoint(
        endpoint, [network, endpoint](const std::string &content,
                                      const std::string &contentType) {
          network->CreateHttpRequest(endpoint)
              ->SetRequestMethod("POST")
              ->SetRequestHeader("Content-Type", contentType)
              ->SetRequestBody(content.c_str(), content.size(), true)
              ->SendRequest([endpoint](auto response) {
                if (response->Code != 200 && response->Code != 204) {
#if defined(_DEBUG)
                  std::ostringstream stream;
                  stream << __FILE__ << "(" << __LINE__ << "): ";
                  stream << "Unable to send logs to remote endpoint "
                         << endpoint << ". status=" << response->Code
                         << ", error=" << response->Error << "\n";
                  OutputDebugString(stream.str().c_str());
#endif
                }
              });
        });
  }
}

void HostBuilder::InitResources(std::string gameUid) {
  if (ParameterManager::Instance().HasParameter(
          ParameterConstants::GAME_DIR_OVERRIDE)) {
    std::string gamesDirOverride(ParameterManager::Instance().GetParameter(
        ParameterConstants::GAME_DIR_OVERRIDE));
    if (gamesDirOverride[gamesDirOverride.length() - 1] != '\\' &&
        gamesDirOverride[gamesDirOverride.length() - 1] != '/') {
      gamesDirOverride.append("\\");
    }
    Resources::Instance().SetGameBaseDir(
        Resources::ToWString(gamesDirOverride));
  }

  const bool userDirOverride = ParameterManager::Instance().HasParameter(
      ParameterConstants::USER_DIR_OVERRIDE);

  Resources::Instance().SetUserBaseDir(gameUid, userDirOverride);
  if (!gameUid.empty()) {
    Resources::Instance().CreateRequiredDirectories();
  }
}

}  // namespace core
}  // namespace MGDF