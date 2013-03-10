#include "stdafx.h"

#include <shlobj.h>
#include <shlwapi.h>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/convenience.hpp>

#include "MGDFVersionInfo.hpp"
#include "MGDFResources.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

Resources::Resources(HINSTANCE instance) {
	if (instance!=nullptr) {
		_applicationDirectory = GetApplicationDirectory(instance);
	}
	else {
		_applicationDirectory = L".";
	}
}

void Resources::SetUserBaseDir(bool useRootDir,const std::string &gameUid)
{
	if (useRootDir) {
		if (!_gameBaseDir.empty())
		{
			boost::filesystem::wpath gamesDirPath(_gameBaseDir,boost::filesystem::native);
			_userBaseDir = gamesDirPath.parent_path().parent_path().native()+L"/user/"+ToWString(gameUid) + (!gameUid.empty() ? L"/" : L"");
		}
		else
		{
			_userBaseDir = _applicationDirectory+L"user/"+ToWString(gameUid) + (!gameUid.empty() ? L"/" : L"");
		}
	}
	else 
	{
		wchar_t strPath[MAX_PATH];
		if(SUCCEEDED(SHGetFolderPathW( nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, strPath ) ) )
		{
			std::wostringstream ss;
			ss << strPath << L"/MGDF/" << MGDFVersionInfo::MGDF_INTERFACE_VERSION << L"/" << ToWString(gameUid) << (!gameUid.empty() ? L"/" : L"");
			_userBaseDir = ss.str();
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
	if (pStr != nullptr)
		*(++pStr)=L'\0';

	std::wstring appDir = szPath;
	boost::algorithm::replace_all(appDir,L"\\",L"/");

	return appDir+L"/";
}

const UINT32 Resources::MIN_SCREEN_X = 1024;
const UINT32 Resources::MIN_SCREEN_Y =768;

std::wstring Resources::LogFile()
{
	return UserBaseDir()+L"coreLog.txt";
}

std::wstring Resources::ParamsFile()
{
	return RootDir() + L"params.txt";
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
	return GameBaseDir() +L"game.json";
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
	return SaveDir(saveName) + L"gameState.json";
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

	boost::filesystem::wpath saveBaseDir(SaveBaseDir(),boost::filesystem::native); 
	boost::filesystem::create_directories(saveBaseDir);

	boost::filesystem::wpath workingDir(WorkingDir(),boost::filesystem::native); 
	boost::filesystem::create_directories(workingDir);
}

std::wstring Resources::CorePreferencesFile()
{
	return _applicationDirectory+L"resources/preferences.json";
}

std::wstring Resources::GameDefaultPreferencesFile()
{
	return GameBaseDir()+  L"preferences.json";
}

std::wstring Resources::GameUserPreferencesFile()
{
	return UserBaseDir() + L"preferences.json";
}

std::wstring Resources::GameUserStatisticsFile() 
{
	return UserBaseDir() + L"statistics.txt";
}

std::wstring Resources::ContentDir()
{
	return GameBaseDir() + L"content/";
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
	INT32 sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);

	std::string result;
	result.resize(sizeNeeded);

	WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), const_cast<LPSTR>(result.data()), sizeNeeded, nullptr, nullptr);
	return result;
}

std::string Resources::ToString(const wchar_t *wstr)
{
	size_t len = wcslen(wstr);
    INT32 sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)len, nullptr, 0, nullptr, nullptr);
	
	std::string result;
	result.resize(sizeNeeded);

    WideCharToMultiByte(CP_UTF8, 0, wstr, (int)len, const_cast<LPSTR>(result.data()), sizeNeeded, nullptr, nullptr);
	return result;
}


std::wstring Resources::ToWString(const std::string &str)
{
    INT32 sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), nullptr, 0);
    
	std::wstring result;
	result.resize(sizeNeeded);

    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), const_cast<LPWSTR>(result.data()), sizeNeeded);
	return result;
}

std::wstring Resources::ToWString(const char *str)
{
	size_t len = strlen(str);
    INT32 sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str, (int)len, nullptr, 0);
	
	std::wstring result;
	result.resize(sizeNeeded);

    MultiByteToWideChar(CP_UTF8, 0, str, (int)len, const_cast<LPWSTR>(result.data()), sizeNeeded);
	return result;
}


}}