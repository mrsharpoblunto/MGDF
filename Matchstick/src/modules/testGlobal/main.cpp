#include "StdAfx.h"

#include <MGDF/MGDF.hpp>

#include "MigratorImpl.hpp"
#include "CustomArchiveHandlersImpl.hpp"

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

MGDF::ICustomArchiveHandlers * MGDF::GetCustomArchiveHandlers(void)
{
	return new CustomArchiveHandlersImpl();
}

MGDF::IGameStateMigrator * MGDF::GetGameStateMigrator(void)
{
	return new MigratorImpl();
}