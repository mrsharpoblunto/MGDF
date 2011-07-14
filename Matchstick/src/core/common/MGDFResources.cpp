#include "stdafx.h"

#include <shlobj.h>
#include <shlwapi.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>

#include "MGDFVersionInfo.hpp"
#include "MGDFResources.hpp"
#include "MGDFUniqueIDAllocator.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

Resources::Resources(HINSTANCE instance) {
	if (instance!=NULL) {
		_applicationDirectory = GetApplicationDirectory(instance);
	}
	else {
		_applicationDirectory = L".";
	}
}

void Resources::SetUserBaseDir(bool useRootDir,const std::string &gameUid)
{
	if (useRootDir) {
		_userBaseDir = (!_gameBaseDir.empty() ? _gameBaseDir : _applicationDirectory)+L"user/"+ToWString(gameUid) + (!gameUid.empty() ? L"/" : L"");
	}
	else 
	{
		wchar_t strPath[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPathW( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strPath ) ) )
		{
			_userBaseDir = strPath;
			_userBaseDir +=L"/MGDF/";
			_userBaseDir +=boost::lexical_cast<std::wstring>(MGDFVersionInfo::MGDF_INTERFACE_VERSION)+L"/"+ ToWString(gameUid) + (!gameUid.empty() ? L"/" : L"");
		}
		else 
		{
			_userBaseDir = (!_gameBaseDir.empty() ? _gameBaseDir : _applicationDirectory)+L"user/"+ToWString(gameUid) + (!gameUid.empty() ? L"/" : L"");
		}
	}
}

/**
get the directory the application is contained within
*/
std::wstring Resources::GetApplicationDirectory(HINSTANCE instance) 
{
	//get the application directory
	wchar_t *pStr, szPath[MAX_PATH];
	GetModuleFileNameW(instance, szPath, MAX_PATH);
	pStr = wcsrchr(szPath, L'\\')-1;
	if (pStr != NULL)
		*(++pStr)=L'\0';

	std::wstring appDir = szPath;
	boost::algorithm::replace_all(appDir,L"\\",L"/");

	return appDir+L"/";
}

const std::wstring Resources::VFS_CONTENT = L"content/";

const std::string Resources::GAME_SCHEMA_URI = "http://schemas.matchstickframework.org/2007/game";
const std::string Resources::GAME_STATE_SCHEMA_URI = "http://schemas.matchstickframework.org/2007/gameState";
const std::string Resources::PREFERENCES_SCHEMA_URI   = "http://schemas.matchstickframework.org/2007/preferences";

const std::wstring Resources::GAME_SCHEMA   = L"game.xsd";
const std::wstring Resources::GAME_STATE_SCHEMA = L"gameState.xsd";
const std::wstring Resources::PREFERENCES_SCHEMA   = L"preferences.xsd";

const unsigned int Resources::MIN_SCREEN_X = 1024;
const unsigned int Resources::MIN_SCREEN_Y =768;

std::wstring Resources::LogFile()
{
	return UserBaseDir()+L"coreLog.txt";
}

std::wstring Resources::RootDir()
{
	return _applicationDirectory;
}

std::wstring Resources::UserBaseDir()
{
	return _userBaseDir;
}

void Resources::SetGameBaseDir(const std::wstring &gameDir)
{
	_gameBaseDir = gameDir;
}

std::wstring Resources::GameBaseDir()
{
	return !_gameBaseDir.empty() ? _gameBaseDir : (_applicationDirectory+L"game/");
}

std::wstring Resources::GameFile()
{
	return GameBaseDir() +L"game.xml";
}

std::wstring Resources::WorkingDir()
{
	return UserBaseDir() + L"working/";
}

std::wstring Resources::SaveBaseDir()
{
	return UserBaseDir()+L"saves/";
}

std::wstring Resources::GameStateSaveFile(const std::string &saveName)
{
	return SaveDir(saveName) + L"gameState.xml";
}

std::wstring Resources::SaveDir(const std::string &saveName)
{
	return SaveBaseDir() +ToWString(saveName)+L"/";
}


std::wstring Resources::SaveDataDir(const std::string &saveName)
{
	return SaveDir(saveName)+L"data/";
}

void Resources::CreateRequiredDirectories()
{
	boost::filesystem::wpath userBaseDir(UserBaseDir(),boost::filesystem::native); 
	boost::filesystem::create_directories(userBaseDir);

	boost::filesystem::wpath gameDir(UserBaseDir(),boost::filesystem::native); 
	boost::filesystem::create_directories(gameDir);

	boost::filesystem::wpath saveBaseDir(SaveBaseDir(),boost::filesystem::native); 
	boost::filesystem::create_directories(saveBaseDir);

	boost::filesystem::wpath workingDir(WorkingDir(),boost::filesystem::native); 
	boost::filesystem::create_directories(workingDir);
}

std::wstring Resources::CorePreferencesFile()
{
	return _applicationDirectory+L"resources/preferences.xml";
}

std::wstring Resources::GameDefaultPreferencesFile()
{
	return GameBaseDir()+  L"preferences.xml";
}

std::wstring Resources::GameUserPreferencesFile()
{
	return UserBaseDir() + L"preferences.xml";
}

std::wstring Resources::GameUserStatisticsFile() 
{
	return UserBaseDir() + L"statistics.txt";
}

std::wstring Resources::ContentDir()
{
	return GameBaseDir() + L"content/";
}

std::wstring Resources::SchemaFile(const std::wstring &schemaFile)
{
	return _applicationDirectory+L"schemas/"+schemaFile;
}

std::wstring Resources::Module()
{
	return BinDir()+L"module.dll";
}

std::wstring Resources::BinDir()
{
	return GameBaseDir()+L"bin/";
}

std::string Resources::ToString(const std::wstring &wstr)
{
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
	char *strTo = new char[sizeNeeded];
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &strTo[0], sizeNeeded, NULL, NULL);
	std::string result(&strTo[0],wstr.size());
	delete[] strTo;
	return result;
}

std::wstring Resources::ToWString(const std::string &str)
{
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    wchar_t *wstrTo = new wchar_t[sizeNeeded];
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstrTo[0], sizeNeeded);
    std::wstring result(&wstrTo[0],str.size());
	delete wstrTo;
	return result;
}


}}