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
	if (_testModule!=nullptr) delete _testModule;

	if (_textManagerCounter) _system->GetTimer()->RemoveCounter(_textManagerCounter);
	if (_testModuleCounter) _system->GetTimer()->RemoveCounter(_testModuleCounter);
}

Module::Module(ISystem *system)
: _textManagerCounter(nullptr)
, _testModuleCounter(nullptr)
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
	if (!_testModuleCounter)
	{
		_testModuleCounter = _system->GetTimer()->CreateCPUCounter("Test Module");
	}

	_testModuleCounter->Begin();
		_testModule->Update(_system,_stateBuffer.Pending());

		TestModule *next = _testModule->NextTestModule();
		if (next!=nullptr)
		{
			delete _testModule;
			_testModule = next;
		}
	_testModuleCounter->End();

	_stateBuffer.Flip();
	return true;
}

bool Module::DrawScene(double alpha)
{
	boost::shared_ptr<TextManagerState> state = _stateBuffer.Interpolate(alpha);
	if (state)
	{
		if (!_textManagerCounter)
		{
			_textManagerCounter = _system->GetTimer()->CreateGPUCounter("Text Rendering");
		}
		_textManagerCounter->Begin();
			_textManager->SetState(state);
			_textManager->DrawText();
		_textManagerCounter->End();
	}
   return true;
}

bool Module::BackBufferChanged()
{
	//_textManager->OnLostDevice();
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