#include "StdAfx.h"

#include "MGDFGameBuilder.hpp"

#include <filesystem>

#include "../common/MGDFParameterManager.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFGameImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::filesystem;

namespace MGDF {
namespace core {

// loads the configuration preferences from the core preferences directory as
// well as the particular configuration defaults, and synchs them up with any
// customized user preferences
MGDFError GameBuilder::LoadGame(
    std::shared_ptr<storage::IStorageFactoryComponent> &storage,
    std::unique_ptr<storage::IGameStorageHandler> &handler,
    ComObject<Game> &game) {
  _ASSERTE(handler);

  Version version;
  handler->GetVersion(version);
  game =
      new Game(handler->GetGameUid(), handler->GetGameName(), version, storage);

  // load the defaults from the core settings (REQUIRED)
  MGDFError err =
      game->LoadPreferences(Resources::Instance().CorePreferencesFile());

  if (MGDF_OK != err) {
    game = nullptr;
    return err;
  }

  // load the defaults for the game if any are present
  game->LoadPreferences(handler->GetPreferences());

  // load customised preferences for this game if any are present
  path customPref(Resources::Instance().GameUserPreferencesFile());

  // then if a settings file exists, override these defaults where present
  // this creates a prefs file with the union of all preferences included but
  // only the most recent values kept (this means it auto updates the
  // preferences listing to include newly added prefs)
  if (exists(customPref)) {
    err = game->LoadPreferences(customPref.wstring());
    if (MGDF_OK != err) {
      LOG("Unable to parse customized preferences "
              << Resources::ToString(customPref.wstring()),
          LOG_ERROR)
    }
  }

  // then save the current preferences as a custom preference file
  // any subsequent changes made by modules will be saved to this file
  game->SavePreferences(customPref.wstring());

  return MGDF_OK;
}

}  // namespace core
}  // namespace MGDF