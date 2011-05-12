#pragma once

#include <MGDF/MGDF.hpp>

class MigratorImpl: public MGDF::DisposeImpl<MGDF::IGameStateMigrator>
{
public:
	virtual ~MigratorImpl(){}
	virtual bool Migrate(const char *moduleName,const char *saveDataDir,const MGDF::Version *currentVersion,const MGDF::Version *requiredVersion);
protected:
	virtual void Dispose();
};