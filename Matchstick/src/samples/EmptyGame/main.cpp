#include "StdAfx.h"

#include <MGDF/MGDF.hpp>
#include "Module.hpp"


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
bool MGDF::IsCompatibleInterfaceVersion( INT32 interfaceVersion )
{
	return interfaceVersion == 1; //compatible with v1 interface
}

//specify to the framework what kind of d3d device features we want/require
UINT32 MGDF::GetCompatibleFeatureLevels( D3D_FEATURE_LEVEL *levels, UINT32 *featureLevelsSize )
{
	//if you want to use the default feature support levels
	//just set featureLevelsSize to 0 and return 0

	//if you want to support a specific feature set (in this example 9_3 only) then first
	//assert the levels array passed in is of the correct size
	if ( *featureLevelsSize != 1 ) {
		//ensure featureLevelsSize is set to the same value as the return value
		*featureLevelsSize = 1;

		//return how large the featureLevels array should be
		return 1;
	} else {
		//specify all the features in descending order of preference
		levels[0] = D3D_FEATURE_LEVEL_9_3;

		//then return 0
		return 0;
	}
}

//create module instances as they are requested by the framework
MGDF::IModule *MGDF::GetModule()
{
	return new Module();
}

//register custom archive handlers
bool MGDF::GetCustomArchiveHandlers( IArchiveHandler **list, UINT32 *length, ILogger *logger, IErrorHandler *errorHandler )
{
	*length = 0;
	return true;
}