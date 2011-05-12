#include "stdafx.h"

#include <shlobj.h>
#include <shlwapi.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>

#include "MGDFVersionInfo.hpp"
#include "MGDFResources.hpp"
#include "MGDFUniqueIDAllocator.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

Resources::Resources(HINSTANCE instance,bool useRootDir) {
	if (instance!=NULL) {
		_applicationDirectory = GetApplicationDirectory(instance);
	}
	else {
		_applicationDirectory = ".";
	}

	SetUserBaseDir(useRootDir);
}

void Resources::SetUserBaseDir(bool useRootDir)
{
	if (useRootDir) {
		_userBaseDir = _applicationDirectory+"user/";
	}
	else 
	{
		char strPath[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath ) ) )
		{
			_userBaseDir = strPath;
			_userBaseDir +="/MGDF/";
			boost::filesystem::path userBaseDir(_userBaseDir,boost::filesystem::native); 
			if (!boost::filesystem::exists(_userBaseDir))
				boost::filesystem::create_directory(_userBaseDir);
			_userBaseDir +=boost::lexical_cast<std::string>(MGDFVersionInfo::MGDF_INTERFACE_VERSION)+"/";
		}
		else 
		{
			_userBaseDir = _applicationDirectory+"user/";
		}
	}
}

/**
get the directory the application is contained within
*/
std::string Resources::GetApplicationDirectory(HINSTANCE instance) 
{
	//get the application directory
	char *pStr, szPath[MAX_PATH];
	GetModuleFileName(instance, szPath, MAX_PATH);
	pStr = strrchr(szPath, '\\')-1;
	if (pStr != NULL)
		*(++pStr)='\0';

	std::string appDir = szPath;
	boost::algorithm::replace_all(appDir,"\\","/");

	return appDir+"/";
}

const std::string Resources::VFS_MODULES = "modules/";
const std::string Resources::VFS_CONTENT = "content/";

const std::string Resources::GAME_SCHEMA_URI = "http://schemas.matchstickframework.org/2007/game";
const std::string Resources::GAME_STATE_SCHEMA_URI = "http://schemas.matchstickframework.org/2007/gameState";
const std::string Resources::PREFERENCES_SCHEMA_URI   = "http://schemas.matchstickframework.org/2007/preferences";

const std::string Resources::GAME_SCHEMA   = "game.xsd";
const std::string Resources::GAME_STATE_SCHEMA = "gameState.xsd";
const std::string Resources::PREFERENCES_SCHEMA   = "preferences.xsd";

const unsigned int Resources::MIN_SCREEN_X = 1024;
const unsigned int Resources::MIN_SCREEN_Y =768;

std::string Resources::LogFile()
{
	return UserBaseDir()+"coreLog.txt";
}

std::string Resources::RootDir()
{
	return _applicationDirectory;
}

std::string Resources::UserBaseDir()
{
	return _userBaseDir;
}

void Resources::SetGamesBaseDir(std::string gameDir)
{
	_gamesBaseDir = gameDir;
}

std::string Resources::GamesBaseDir()
{
	return _gamesBaseDir!="" ? _gamesBaseDir : (_applicationDirectory+"games/");
}

std::string Resources::UserDir(std::string gameUid)
{
	return UserBaseDir()+"games/"+gameUid+"/";
}

std::string Resources::GameFile(std::string gameUid)
{
	return GamesBaseDir() + gameUid +"/game.xml";
}

std::string Resources::BootDir(std::string gameUid)
{
	return GamesBaseDir()+gameUid+"/boot/";
}

std::string Resources::WorkingDir()
{
	return UserBaseDir() + "working/";
}

std::string Resources::GameStateBootFile(std::string gameUid)
{
	return BootDir(gameUid) + "gameState.xml";
}

std::string Resources::GameStateSaveFile(std::string gameUid,std::string saveName)
{
	return SaveDir(gameUid,saveName) + "gameState.xml";
}

std::string Resources::RelativeSaveFile()
{
	return boost::lexical_cast<std::string>(UniqueIDAllocator::GetID())+".sav";
}

std::string Resources::SaveFile(std::string gameUid,std::string saveName)
{
	return SaveDir(gameUid,saveName)+boost::lexical_cast<std::string>(UniqueIDAllocator::GetID())+".sav";
}

std::string Resources::SaveDir(std::string gameUid,std::string saveName)
{
	return UserDir(gameUid)+saveName+"/";
}

std::string Resources::SaveDataDir(std::string gameUid,std::string saveName)
{
	return SaveDir(gameUid,saveName)+"data/";
}

std::string Resources::BootDataDir(std::string gameUid)
{
	return BootDir(gameUid)+"data/";
}

void Resources::CreateRequiredDirectories(std::string gameUid)
{
	boost::filesystem::path userBaseDir(UserBaseDir(),boost::filesystem::native); 
	if (!exists(userBaseDir))
		create_directory(userBaseDir);
	boost::filesystem::path userGamesBaseDir(UserBaseDir()+"games",boost::filesystem::native); 
	if (!exists(userGamesBaseDir))
		create_directory(userGamesBaseDir);
	boost::filesystem::path gameDir(UserDir(gameUid),boost::filesystem::native); 
	if (!exists(gameDir))
		create_directory(gameDir);
	boost::filesystem::path workingDir(Resources::Instance().WorkingDir(),boost::filesystem::native); 
	if (exists(workingDir)) {
		create_directory(workingDir);
	}
}

std::string Resources::CorePreferencesFile()
{
	return _applicationDirectory+"games/core/preferences.xml";
}

std::string Resources::GameDefaultPreferencesFile(std::string gameUid)
{
	return GamesBaseDir() + gameUid + "/preferences.xml";
}

std::string Resources::GameUserPreferencesFile(std::string gameUid)
{
	return UserDir(gameUid) + "preferences.xml";
}

std::string Resources::GameUserStatisticsFile(std::string gameUid) 
{
	return UserDir(gameUid) + "statistics.txt";
}

std::string Resources::ContentDir(std::string gameUid)
{
	return GamesBaseDir() + gameUid + "/content/";
}

std::string Resources::SchemaFile(std::string schemaFile)
{
	return _applicationDirectory+"schemas/"+schemaFile;
}

std::string Resources::GlobalModule(std::string gameUid)
{
	return GamesBaseDir()+gameUid+"/modules/global.dll";
}

std::string Resources::ModulesDir(std::string gameUid)
{
	return GamesBaseDir()+gameUid+"/modules/";
}

}}