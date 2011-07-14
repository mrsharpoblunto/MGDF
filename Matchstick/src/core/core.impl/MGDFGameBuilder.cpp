#include "StdAfx.h"

#include "MGDFGameBuilder.hpp"
#include <boost/filesystem/operations.hpp>
#include "MGDFGameImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../common/MGDFParameterManagerImpl.hpp"
#include "MGDFComponents.hpp"
#include "../xml/MGDFXMLFactoryComponentImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

//loads a boot configuration from a file in the boot folder
Game *GameBuilder::LoadGame()
{	
	xml::IXMLFactoryComponent *xmlFactory = Components::Instance().Get<xml::IXMLFactoryComponent>();

	std::auto_ptr<xml::IGameXMLHandler> handler(xmlFactory->CreateGameXMLHandler());
	handler->Load(Resources::Instance().GameFile());

	//add this configurations parameters to the parameter manager
	GetParameterManagerImpl()->AddParameterString(handler->GetParameterString().c_str());

	//initialise the configuration preferences
	Game *game = CreateGame(handler->GetGameUid(),handler->GetGameName(),handler->GetInterfaceVersion(),handler->GetVersion());

	return game;
}


//loads the configuration preferences from the core preferences directory as well as the 
//particular configuration defaults, and synchs them up with any customized user preferences
Game *GameBuilder::CreateGame(std::string uid,std::string name,int interfaceVersion,const Version *version)
{
	xml::IXMLFactoryComponent *xmlFactory = Components::Instance().Get<xml::IXMLFactoryComponent>();

	Game *game = new Game(uid,name,interfaceVersion,version,xmlFactory);
	
	//load the defaults from the core settings and the game settings (REQUIRED)
	game->LoadPreferences(Resources::Instance().CorePreferencesFile());
	
	//load the defaults for the game (OPTIONAL)
	boost::filesystem::wpath bootDefaultPref(
		Resources::Instance().GameDefaultPreferencesFile(),boost::filesystem::native
	);

	if (exists(bootDefaultPref)) {
		game->LoadPreferences(bootDefaultPref.native_file_string());
	}

	//load customised preferences for this game (OPTIONAL)
	boost::filesystem::wpath customPref(
		Resources::Instance().GameUserPreferencesFile(),boost::filesystem::native
	);

	//then if a settings file exists, override these defaults where present
	//this creates a prefs file with the union of all preferences included but only the 
	//most recent values kept (this means it auto updates the preferences listing to include newly added prefs) 
	if (exists(customPref)) {
		game->LoadPreferences(customPref.native_file_string());
	}

	//then save the current preferences as a custom preference file
	//any subsequent changes made by modules will be saved to this file
	game->SavePreferences(customPref.native_file_string());

	return game;
}


}}