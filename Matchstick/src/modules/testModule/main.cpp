#include "StdAfx.h"

#include <MGDF/MGDF.hpp>
#include "Module.hpp"
#include "CustomArchiveHandlersImpl.hpp"
#include "MigratorImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

// Function that returns if this module is compatible with the framework calling it
bool MGDF::IsCompatibleInterfaceVersion(int interfaceVersion) {
	return interfaceVersion==1;//compatible with v1 interface
}

//create module instances as they are requested by the system.
MGDF::IModule *MGDF::GetModule(MGDF::ISystem *system) {
	return new MGDF::Test::Module(system);
}

//register custom archive handlers
MGDF::ICustomArchiveHandlers * MGDF::GetCustomArchiveHandlers(void)
{
	return new CustomArchiveHandlersImpl();
}

//provide gamestate migrator
MGDF::IGameStateMigrator * MGDF::GetGameStateMigrator(void)
{
	return new MigratorImpl();
}