#pragma once

#include <MGDF/MGDFDisposable.hpp>
#include <MGDF/MGDFVersion.hpp>

namespace MGDF {

/**
every game can have its own gameStateMigrator, the purpose of this object is to update old save files
to the required version, this is done so that specific data fixes can be made independantly of the game update process
in a lazy fashion
*/
	class IGameStateMigrator: public IDisposable
{
public:
	/**
	migrate a save file for a particular module from a particular version of this game to a newer version save file
	return true if the migration succeeds, false if it fails If any changes are required it is the responsibility of the migrator to change the
	save file and save the changes.
	*/
	virtual bool Migrate(const wchar_t *saveDataDir,const Version *currentVersion,const Version *requiredVersion)=0;
};

/**
exports the getmodule function so the system can get an instance of a gamestate migrator
of a module
*/
extern "C" __declspec(dllexport) IGameStateMigrator * GetGameStateMigrator(void);

}
