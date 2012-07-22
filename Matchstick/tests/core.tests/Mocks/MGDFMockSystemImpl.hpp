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
	virtual void QueueSaveGameState(const wchar_t *) {}
	virtual void QueueLoadGameState(const wchar_t *) {}
	virtual ILogger *GetLogger() const { return nullptr; }
	virtual IParameterManager *GetParameters() const { return nullptr; }
	virtual IVirtualFileSystem *GetVFS() const { return nullptr; }
	virtual ISoundManager *GetSound() const { return nullptr; }
	virtual IGame *GetGame() const { return nullptr; }
	virtual IGraphicsManager *GetGraphics() const { return nullptr; }
	virtual IInputManager *GetInput() const { return nullptr; }
	virtual IStatisticsManager *GetStatistics() const { return nullptr; }
	virtual ID3D11Device *GetD3DDevice() const { return nullptr; }
	virtual IStringList *ListSaved() const { return new StringList(); }
	virtual const Error *GetLastError() const { return &_lastError; }
private:
	Error _lastError;
	Version _version;
};

}}}