#include "StdAfx.h"

#include <MGDF/MGDF.hpp>
#include "test1.hpp"
#include "test2.hpp"

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
	if (moduleName=="Test1") {
		MGDF::IModule *m = new MGDF::Test::Test1(system);
		return m;
	}
	else if (moduleName=="Test2") {
		MGDF::IModule *m = new MGDF::Test::Test2(system);
		return m;
	}
	return NULL;
}