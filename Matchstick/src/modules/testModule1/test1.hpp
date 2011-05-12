#pragma once

#include <time.h>
#include "d3d9.h"
#include <MGDF/MGDF.hpp>

#include "BufferedGameState.hpp"
#include "TextManager.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

class Test1: public IModule 
{
public:
	virtual ~Test1(void);
	Test1(ISystem *);

	virtual bool NewModule(const char *workingFolder,const IModuleInitialiser *init);
	virtual bool LoadModule(const char *,const char *,const IModuleInitialiser *);//file to load from
	
	virtual bool Dispose(void);

	virtual bool DrawScene(double elapsedTime);
	virtual bool DeviceLost();
	virtual bool DeviceReset();
	virtual bool SetDeviceState();
	virtual bool CheckDeviceCaps();

	virtual bool UpdateScene(double alpha);
	virtual bool Suspend();
	virtual bool Resume();
	virtual bool SaveModule(const char *);//file to save to
	virtual void Panic();

	const char *GetLastError();
private:
	MGDF::ISystem *_system;
	bool _inited;
	std::string _lastError,_workingFolder;

	BufferedGameState<TextManagerState> _stateBuffer;
	TextManager *_textManager;
	int _testState;
	time_t _time;
};

}}
