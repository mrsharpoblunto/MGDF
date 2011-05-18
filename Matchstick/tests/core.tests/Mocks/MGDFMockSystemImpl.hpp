#pragma once

#include <list>
#include "../../../src/core/core.impl/MGDFSystemImpl.hpp"

namespace MGDF { namespace core { namespace tests {

/**
mock system used to test out event listener interfaces for internal components which rely on those events to function correctly
*/
class MockSystemImpl: public ISystemImpl {
public:
	MockSystemImpl();
	virtual ~MockSystemImpl(void);

	IModule * _module;
	virtual IModule *GetModule();
	
	virtual void FatalError(const char *,const char *);
	virtual void SetLastError(const char *,int,const char *);

	virtual void ShutDown();
	virtual const Version * GetMGDFVersion() const;

	//these functions are not required for the testing being performed
	virtual void QueueSaveGameState(const char *) {}
	virtual void QueueLoadGameState(const char *) {}
	virtual ILogger *GetLogger() const { return NULL; }
	virtual IParameterManager *GetParameters() const { return NULL; }
	virtual IVirtualFileSystem *GetVFS() const { return NULL; }
	virtual ISoundManager *GetSound() const { return NULL; }
	virtual IGame *GetGame() const { return NULL; }
	virtual IGraphicsManager *GetGraphics() const { return NULL; }
	virtual IInputManager *GetInput() const { return NULL; }
	virtual IStatisticsManager *GetStatistics() const { return NULL; }
	virtual IDirect3DDevice9 *GetD3DDevice() const { return NULL; }
	virtual IStringList *ListSaved() const { return new StringList(); }
	virtual const Error *GetLastError() const { return &_lastError; }
private:
	Error _lastError;
	Version _version;
};

}}}