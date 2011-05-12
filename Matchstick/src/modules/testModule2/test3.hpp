#pragma once

#include "d3d9.h"
#include "TextManager.hpp"
#include "BufferedGameState.hpp"
#include <MGDF/MGDF.hpp>

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

/**
this class acts as a command console to the lua system interfaces
*/
class Test3: public IModule 
{
public:
	virtual ~Test3(void);
	Test3(ISystem *);

	virtual bool NewModule(const char *workingFolder,const IModuleInitialiser *init);
	virtual bool LoadModule(const char *,const char *,const IModuleInitialiser *);//file to load from
	
	virtual bool Dispose(void);

	virtual bool DrawScene(double alpha);
	virtual bool DeviceLost();
	virtual bool DeviceReset();
	virtual bool SetDeviceState();
	virtual bool CheckDeviceCaps();

	virtual bool UpdateScene(double elapsedTime);
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
	BufferedGameState<TextManagerState> _state;

};

}}
