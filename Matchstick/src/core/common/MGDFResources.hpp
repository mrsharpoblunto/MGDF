#pragma once

#include <windows.h>
#include <string>
#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

class MGDF_CORE_COMMON_DLL Resources
{
public:
	virtual ~Resources(){}

	static Resources *InstancePtr(HINSTANCE instance=NULL) {
		static Resources r(instance);
		return &r;
	}

	static Resources &Instance(HINSTANCE instance=NULL) {
		return *Resources::InstancePtr(instance);
	}

	std::wstring RootDir();

	void CreateRequiredDirectories();
	void SetUserBaseDir(bool useRootDir,const std::string &gameUid);
	void SetGameBaseDir(const std::wstring &gameDir);

	std::wstring GameBaseDir();
	std::wstring UserBaseDir();

	std::wstring GameFile();
	std::wstring WorkingDir();
	std::wstring SaveBaseDir();
	std::wstring GameStateSaveFile(const std::string &saveName);
	std::wstring SaveDir(const std::string& saveName);
	std::wstring SaveDataDir(const std::string &saveName);
	std::wstring CorePreferencesFile();
	std::wstring GameDefaultPreferencesFile();
	std::wstring GameUserPreferencesFile();
	std::wstring GameUserStatisticsFile();
	std::wstring ContentDir();
	std::wstring SchemaFile(const std::wstring &schemaFile);
	std::wstring Module();
	std::wstring BinDir();
	std::wstring LogFile();

	static const std::string GAME_SCHEMA_URI;
	static const std::string GAME_STATE_SCHEMA_URI;
	static const std::string PREFERENCES_SCHEMA_URI;

	static const std::wstring GAME_SCHEMA;
	static const std::wstring GAME_STATE_SCHEMA;
	static const std::wstring PREFERENCES_SCHEMA;

	static const std::wstring VFS_CONTENT;

	static const unsigned int MIN_SCREEN_X;
	static const unsigned int MIN_SCREEN_Y;

	static std::wstring ToWString(const std::string &str);
	static std::string ToString(const std::wstring &str);
private: 
	Resources(HINSTANCE instance);
	static std::wstring GetApplicationDirectory(HINSTANCE instance);
	std::wstring _applicationDirectory,_userBaseDir,_gameBaseDir;
};

}}
