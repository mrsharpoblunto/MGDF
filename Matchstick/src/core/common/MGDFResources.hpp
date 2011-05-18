#pragma once

#include <windows.h>
#include <string>
#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core {

class MGDF_CORE_COMMON_DLL Resources
{
public:
	virtual ~Resources(){}

	static Resources *InstancePtr(HINSTANCE instance=NULL,bool useRootDir=NULL) {
		static Resources r(instance,useRootDir);
		return &r;
	}

	static Resources &Instance(HINSTANCE instance=NULL,bool useRootDir=NULL) {
		return *Resources::InstancePtr(instance,useRootDir);
	}

	std::string RootDir();

	void CreateRequiredDirectories(std::string gameUid);
	void SetUserBaseDir(bool useRootDir);
	void SetGamesBaseDir(std::string gameDir);

	std::string GamesBaseDir();
	std::string UserBaseDir();
	std::string UserDir(std::string configurationName);
	std::string GameFile(std::string gameFile);
	std::string WorkingDir();
	std::string GameStateSaveFile(std::string gameUid,std::string saveName);
	std::string RelativeSaveFile();
	std::string SaveFile(std::string gameUid,std::string saveName);
	std::string SaveDir(std::string gameUid,std::string saveName);
	std::string SaveDataDir(std::string gameUid,std::string saveName);
	std::string CorePreferencesFile();
	std::string GameDefaultPreferencesFile(std::string gameUid);
	std::string GameUserPreferencesFile(std::string gameUid);
	std::string GameUserStatisticsFile(std::string gameUid);
	std::string ContentDir(std::string gameUid);
	std::string SchemaFile(std::string schemaFile);
	std::string Module(std::string gameUid);
	std::string BinDir(std::string gameUid);
	std::string LogFile();

	static const std::string GAME_SCHEMA_URI;
	static const std::string GAME_STATE_SCHEMA_URI;
	static const std::string PREFERENCES_SCHEMA_URI;

	static const std::string GAME_SCHEMA;
	static const std::string GAME_STATE_SCHEMA;
	static const std::string PREFERENCES_SCHEMA;

	static const std::string VFS_CONTENT;

	static const unsigned int MIN_SCREEN_X;
	static const unsigned int MIN_SCREEN_Y;
private: 
	Resources(HINSTANCE instance,bool useAppRoot);
	static std::string GetApplicationDirectory(HINSTANCE instance);
	std::string _applicationDirectory,_userBaseDir,_gamesBaseDir;
};

}}
