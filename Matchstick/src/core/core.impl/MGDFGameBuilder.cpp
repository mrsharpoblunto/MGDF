#include "StdAfx.h"

#include "MGDFGameBuilder.hpp"
#include <boost/filesystem/operations.hpp>
#include "MGDFGameImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFParameterManager.hpp"
#include "MGDFComponents.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

//loads a game from the game.json file stored in the game folder
Game *GameBuilder::LoadGame( storage::IGameStorageHandler *handler )
{
	_ASSERTE( handler );

	ParameterManager::Instance().AddParameterString( handler->GetParameterString().c_str() );
	Game *game = CreateGame(
	                 handler->GetGameUid(),
	                 handler->GetGameName(),
	                 handler->GetInterfaceVersion(),
	                 handler->GetVersion() );
	return game;
}


//loads the configuration preferences from the core preferences directory as well as the
//particular configuration defaults, and synchs them up with any customized user preferences
Game *GameBuilder::CreateGame( const std::string &uid, const std::string &name, INT32 interfaceVersion, const Version *version )
{
	_ASSERTE( version );

	storage::IStorageFactoryComponent *storageFactory = Components::Instance().Get<storage::IStorageFactoryComponent>();

	Game *game = new Game( uid, name, interfaceVersion, version, storageFactory );

	//load the defaults from the core settings and the game settings (REQUIRED)
	game->LoadPreferences( Resources::Instance().CorePreferencesFile() );

	//load the defaults for the game (OPTIONAL)
	boost::filesystem::wpath bootDefaultPref(
	    Resources::Instance().GameDefaultPreferencesFile(), boost::filesystem::native
	);

	if ( exists( bootDefaultPref ) ) {
		game->LoadPreferences( bootDefaultPref.native() );
	}

	//load customised preferences for this game (OPTIONAL)
	boost::filesystem::wpath customPref(
	    Resources::Instance().GameUserPreferencesFile(), boost::filesystem::native
	);

	//then if a settings file exists, override these defaults where present
	//this creates a prefs file with the union of all preferences included but only the
	//most recent values kept (this means it auto updates the preferences listing to include newly added prefs)
	if ( exists( customPref ) ) {
		game->LoadPreferences( customPref.native() );
	}

	//then save the current preferences as a custom preference file
	//any subsequent changes made by modules will be saved to this file
	game->SavePreferences( customPref.native() );

	return game;
}


}
}