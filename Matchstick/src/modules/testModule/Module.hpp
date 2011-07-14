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

class TestModule
{
public:
	virtual ~TestModule(void){}

	virtual void Update(ISystem *system,TextManagerState *state)=0;
	virtual void LoadModule(const wchar_t *,const wchar_t *,TextManagerState *state)=0;
	virtual void SaveModule(const wchar_t *,TextManagerState *state)=0;

	virtual TestModule *NextTestModule()=0;
};

class Module: public IModule 
{
public:
	virtual ~Module(void);
	Module(ISystem *);

	virtual bool NewModule(const wchar_t *workingFolder);
	virtual bool LoadModule(const wchar_t *,const wchar_t *);//file to load from
	virtual bool SaveModule(const wchar_t *);//file to save to

	virtual bool Dispose(void);

	virtual bool DrawScene(double alpha);
	virtual bool DeviceLost();
	virtual bool DeviceReset();
	virtual bool SetDeviceState();
	virtual bool CheckDeviceCaps();

	virtual bool UpdateScene(double elapsedTime);
	virtual void Panic();

	const char *GetLastError();
private:
	MGDF::ISystem *_system;
	bool _inited;
	std::string _lastError;
	std::wstring _workingFolder;

	TestModule *_testModule;
	BufferedGameState<TextManagerState> _stateBuffer;
	TextManager *_textManager;
};

}}
