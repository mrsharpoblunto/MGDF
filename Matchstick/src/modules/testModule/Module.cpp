#include "StdAfx.h"

#include "Test1.hpp"
#include "Test3.hpp"
#include <fstream>

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF { namespace Test {

Module::~Module(void)
{
	delete _textManager;
	if (_testModule!=NULL) delete _testModule;
}

Module::Module(ISystem *system)
{
	_system = system;
	_textManager = new TextManager(system);
	_stateBuffer.Pending()->AddLine(WHITE,"MGDF functional test suite started");
}

bool Module::New(const wchar_t *workingFolder)
{
	_workingFolder = workingFolder;
	_testModule = new Test1();

	return true;
}

bool Module::Dispose(void)
{
	delete this;
	return true;
}

bool Module::UpdateScene(double elapsedTime)
{
	_testModule->Update(_system,_stateBuffer.Pending());

	TestModule *next = _testModule->NextTestModule();
	if (next!=NULL)
	{
		delete _testModule;
		_testModule = next;
	}

	_stateBuffer.Flip();
	return true;
}

bool Module::DrawScene(double alpha)
{
	boost::shared_ptr<TextManagerState> state = _stateBuffer.Interpolate(alpha);
	if (state)
	{
		_textManager->SetState(state);
		_textManager->DrawText();
	}
   return true;
}

bool Module::DeviceReset()
{
	_textManager->OnResetDevice();
	return true;
}

bool Module::SetDeviceState()
{
	return true;
}

bool Module::DeviceLost()
{
	_textManager->OnLostDevice();
	return true;
}

bool Module::CheckDeviceCaps() {
	return true;
}

void Module::ShutDown()
{
	_system->ShutDown();
}

void Module::Panic()
{
}

const char * Module::GetLastError()
{
	return _lastError.c_str();
}

}}