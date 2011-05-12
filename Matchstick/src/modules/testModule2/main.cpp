#include "StdAfx.h"

#include <MGDF/MGDF.hpp>
#include "Test3.hpp"

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
MGDF::IModule *MGDF::GetModule(const char *name,MGDF::ISystem *system) {
	std::string moduleName = name;
	if (moduleName=="Test3") {
		MGDF::IModule *m = new MGDF::Test::Test3(system);
		return m;
	}
	return NULL;
}