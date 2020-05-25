#include "StdAfx.h"

#include "MGDFHostBuilder.hpp"

#include <MGDF/MGDF.hpp>
#include <filesystem>
#include <fstream>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFStringImpl.hpp"
#include "../common/MGDFVersionInfo.hpp"
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
  InitLogger();

  storage::IStorageFactoryComponent *storageImpl =
      storage::CreateStorageFactoryComponentImpl();
  if (storageImpl != nullptr) {
    components.Storage = storageImpl;
  } else {
    LOG("FATAL ERROR: Unable to register StorageFactory", LOG_ERROR);
    return false;
  }

  return true;
}

bool HostBuilder::RegisterAdditionalComponents(std::string gameUid,
                                               HostComponents &components) {
  auto input = input::CreateInputManagerComponentImpl();
  if (input) {
    components.Input = input;
  } else {
    LOG("FATAL ERROR: Unable to register InputManager", LOG_ERROR);
    return false;
  }

  auto vfs = vfs::CreateVirtualFileSystemComponentImpl();
  if (vfs) {
    components.VFS = vfs;
  } else {
    LOG("FATAL ERROR: Unable to register VirtualFileSystem", LOG_ERROR);
    return false;
  }

  auto audioImpl = audio::CreateSoundManagerComponentImpl();
  if (audioImpl) {
    components.Sound = audioImpl;
  } else {
    // its a problem, but we can still probably run if the soundmanager failed
    // to initialize
    LOG("ERROR: Unable to register SoundManager", LOG_ERROR);
  }

  return true;
}

void HostBuilder::UnregisterComponents(HostComponents &components) {
  // TODO once these are all COM objects -> remove this...
  SAFE_DELETE(components.Storage);
}

MGDFError HostBuilder::TryCreateHost(Host **host) {
  HostComponents components;
  memset(&components, 0, sizeof(HostComponents));

  // do the bare minimum setup required to be able to
  // load up the game configuration file
  if (!RegisterBaseComponents(components)) {
    return MGDF_ERR_FATAL;
  }

  // try and load the game configuration file
  _ASSERTE(components.Storage);

  std::unique_ptr<storage::IGameStorageHandler> handler(
      components.Storage->CreateGameStorageHandler());
  _ASSERTE(handler.get());

  MGDFError result = handler->Load(Resources::Instance().GameFile());
  if (MGDF_OK != result) {
    UnregisterComponents(components);
    return result;
  }

  // now that we know the UID for the game, we'll set up the user resources
  // paths again and shift the logfile over to the new user directory
  InitResources(handler->GetGameUid());
  Logger::Instance().MoveOutputFile();

  ComObject<Game> game;
  result = GameBuilder::LoadGame(components.Storage, handler.get(), game);
  if (MGDF_OK != result) {
    LOG("FATAL ERROR: Unable to load game configuration", LOG_ERROR);
    UnregisterComponents(components);
    return result;
  }

  // now that the game file loaded, initialize everything else
  // and set the log directory correctly.
  if (!RegisterAdditionalComponents(game->GetUid(), components)) {
    UnregisterComponents(components);
    return MGDF_ERR_FATAL;
  }

  LOG("Creating host...", LOG_LOW);
  result = Host::TryCreate(game, components, host);
  if (MGDF_OK != result) {
    LOG("FATAL ERROR: Unable to create host", LOG_ERROR);
    UnregisterComponents(components);
    return result;
  }

  return MGDF_OK;
}

void HostBuilder::DisposeHost(Host *host) {
  if (host != nullptr) {
    host->STDisposeModule();
  }
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
    if (paramString.starts_with("//")) {
      paramString.clear();
    }
  }

  // otherwise parse the command line
  if (paramString.empty()) {
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