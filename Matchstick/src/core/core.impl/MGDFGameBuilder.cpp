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
HRESULT GameBuilder::LoadGame(
    std::shared_ptr<storage::IStorageFactoryComponent> &storage,
    const std::unique_ptr<storage::IGameStorageHandler> &handler,
    ComObject<Game> &game) {
  _ASSERTE(handler);

  MGDFVersion version;
  handler->GetVersion(version);
  game = MakeCom<Game>(handler->GetGameUid(), handler->GetGameName(), version,
                       storage);

  // load the defaults from the core settings (REQUIRED)
  auto result =
      game->LoadPreferences(Resources::Instance().CorePreferencesFile());

  if (FAILED(result)) {
    game.Clear();
    return result;
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
    result = game->LoadPreferences(customPref.wstring());
    if (FAILED(result)) {
      LOG("Unable to parse customized preferences "
              << Resources::ToString(customPref.wstring()),
          MGDF_LOG_ERROR);
    }
  }

  // then save the current preferences as a custom preference file
  // any subsequent changes made by modules will be saved to this file
  game->SavePreferences(customPref.wstring());

  return S_OK;
}

}  // namespace core
}  // namespace MGDF