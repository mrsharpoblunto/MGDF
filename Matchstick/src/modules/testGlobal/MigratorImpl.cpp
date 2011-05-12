#include "StdAfx.h"

#include "MigratorImpl.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

bool MigratorImpl::Migrate(const char *moduleName,const char *saveDataDir,const MGDF::Version *currentVersion,const MGDF::Version *requiredVersion)
{
	//doesn't do anything atm.
	return true;
}

void MigratorImpl::Dispose(void)
{
	delete this;
}