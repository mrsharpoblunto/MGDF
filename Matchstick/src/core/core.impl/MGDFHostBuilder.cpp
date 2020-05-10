#include "StdAfx.h"

#include "MGDFHostBuilder.hpp"

#include <MGDF/MGDF.hpp>
#include <filesystem>
#include <fstream>

#include "../audio/MGDFSoundManagerComponentImpl.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFVersionInfo.hpp"
#include "../input/MGDFInputManagerComponentImpl.hpp"
#include "../storage/MGDFStorageFactoryComponentImpl.hpp"
#include "../vfs/MGDFVirtualFileSystemComponentImpl.hpp"
#include "MGDFComponents.hpp"
#include "MGDFGameBuilder.hpp"
#include "MGDFParameterConstants.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::filesystem;

namespace MGDF {
namespace core {

bool HostBuilder::RegisterBaseComponents() {
  // init global common components
  InitParameterManager();
  InitResources();
  InitLogger();

  storage::IStorageFactoryComponent *storageImpl =
      storage::CreateStorageFactoryComponentImpl();
  if (storageImpl != nullptr) {
    Components::Instance().RegisterComponent<storage::IStorageFactoryComponent>(
        storageImpl);
  } else {
    LOG("FATAL ERROR: Unable to register StorageFactory", LOG_ERROR);
    return false;
  }

  return true;
}

bool HostBuilder::RegisterAdditionalComponents(std::string gameUid) {
  input::IInputManagerComponent *input =
      input::CreateInputManagerComponentImpl();
  if (input != nullptr) {
    Components::Instance().RegisterComponent<input::IInputManagerComponent>(
        input);
  } else {
    LOG("FATAL ERROR: Unable to register InputManager", LOG_ERROR);
    return false;
  }

  vfs::IVirtualFileSystemComponent *vfs =
      vfs::CreateVirtualFileSystemComponentImpl();
  if (vfs != nullptr) {
    Components::Instance().RegisterComponent<vfs::IVirtualFileSystemComponent>(
        vfs);
  } else {
    LOG("FATAL ERROR: Unable to register VirtualFileSystem", LOG_ERROR);
    return false;
  }

  audio::ISoundManagerComponent *audioImpl =
      audio::CreateSoundManagerComponentImpl(vfs);
  if (audioImpl != nullptr) {
    Components::Instance().RegisterComponent<audio::ISoundManagerComponent>(
        audioImpl);
  } else {
    // its a problem, but we can still probably run if the soundmanager failed
    // to initialize
    LOG("ERROR: Unable to register SoundManager", LOG_ERROR);
  }

  return true;
}

void HostBuilder::UnregisterComponents() {
  Components::Instance()
      .UnregisterComponent<storage::IStorageFactoryComponent>();
  Components::Instance().UnregisterComponent<input::IInputManagerComponent>();
  Components::Instance().UnregisterComponent<audio::ISoundManagerComponent>();
  Components::Instance()
      .UnregisterComponent<vfs::IVirtualFileSystemComponent>();
}

MGDFError HostBuilder::TryCreateHost(Host **host) {
  // do the bare minimum setup required to be able to
  // load up the game configuration file
  if (!RegisterBaseComponents()) {
    return MGDF_ERR_FATAL;
  }

  // try and load the game configuration file
  storage::IStorageFactoryComponent *storageFactory =
      Components::Instance().Get<storage::IStorageFactoryComponent>();
  _ASSERTE(storageFactory);

  std::unique_ptr<storage::IGameStorageHandler> handler(
      storageFactory->CreateGameStorageHandler());
  _ASSERTE(handler.get());

  MGDFError result = handler->Load(Resources::Instance().GameFile());
  if (MGDF_OK != result) {
    UnregisterComponents();
    return result;
  }

  // now that we know the UID for the game, we'll set up the user resources
  // paths again and shift the logfile over to the new user directory
  InitResources(handler->GetGameUid());
  Logger::Instance().MoveOutputFile();

  Game *game;
  result = GameBuilder::LoadGame(handler.get(), &game);
  if (MGDF_OK != result) {
    LOG("FATAL ERROR: Unable to load game configuration", LOG_ERROR);
    UnregisterComponents();
    return result;
  }

  // now that the game file loaded, initialize everything else
  // and set the log directory correctly.
  if (!RegisterAdditionalComponents(game->GetUid())) {
    UnregisterComponents();
    return MGDF_ERR_FATAL;
  }

  if (MGDFVersionInfo::MGDF_INTERFACE_VERSION != game->GetInterfaceVersion()) {
    LOG("FATAL ERROR: Unsupported MGDF Interface version", LOG_ERROR);
    delete game;
    UnregisterComponents();
    return MGDF_ERR_FATAL;
  }

  LOG("Creating host...", LOG_LOW);
  result = Host::TryCreate(game, host);
  if (MGDF_OK != result) {
    LOG("FATAL ERROR: Unable to create host", LOG_ERROR);
    UnregisterComponents();
    return result;
  }

  Components::Instance().RegisterComponentErrorHandler(
      *host);  // register the hosterror handlers with all components
  return MGDF_OK;
}

void HostBuilder::DisposeHost(Host *host) {
  if (host != nullptr) {
    host->STDisposeModule();
  }
  UnregisterComponents();
  SAFE_DELETE(host);
}

void HostBuilder::InitParameterManager() {
  std::string paramString;

  // use the supplied params.txt in the application path (if provided)
  // providing a params.txt can be useful for debugging purposes.
  path paramsTxt(Resources::Instance().ParamsFile());
  if (exists(paramsTxt)) {
    std::ifstream input(paramsTxt.wstring().c_str(), std::ios::in);
    std::stringstream buffer;
    buffer << input.rdbuf();
    paramString = buffer.str();
  }
  // otherwise parse the command line
  else {
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
  }

  // add the parameters to the parameter manager
  ParameterManager::Instance().AddParameterString(paramString.c_str());
}

void HostBuilder::InitLogger() {
  if (ParameterManager::Instance().HasParameter(
          ParameterConstants::LOG_LEVEL)) {
    const char *level = ParameterManager::Instance().GetParameter(
        ParameterConstants::LOG_LEVEL);

    if (level == ParameterConstants::VALUE_LOG_LEVEL_ERROR) {
      Logger::Instance().SetLoggingLevel(LOG_ERROR);
    }
    if (level == ParameterConstants::VALUE_LOG_LEVEL_LOW) {
      Logger::Instance().SetLoggingLevel(LOG_LOW);
    }
    if (level == ParameterConstants::VALUE_LOG_LEVEL_MEDIUM) {
      Logger::Instance().SetLoggingLevel(LOG_MEDIUM);
    }
    if (level == ParameterConstants::VALUE_LOG_LEVEL_HIGH) {
      Logger::Instance().SetLoggingLevel(LOG_HIGH);
    }
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

  bool userDirOverride = ParameterManager::Instance().HasParameter(
      ParameterConstants::USER_DIR_OVERRIDE);

  Resources::Instance().SetUserBaseDir(userDirOverride, gameUid);
  if (!gameUid.empty()) {
    Resources::Instance().CreateRequiredDirectories();
  }
}

}  // namespace core
}  // namespace MGDF