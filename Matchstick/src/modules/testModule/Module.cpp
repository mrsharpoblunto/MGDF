#include "StdAfx.h"

#include "Test1.hpp"
#include "Test3.hpp"
#include <fstream>


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace Test
{

Module::~Module( void )
{
	delete _textManager;
	delete _testModule;

	if ( _textManagerCounter ) _textManagerCounter->Release();
	if ( _testModuleCounter ) _testModuleCounter->Release();
}

Module::Module( )
	: _textManagerCounter( nullptr )
	, _testModuleCounter( nullptr )
	, _textManager( nullptr )
	, _testModule( nullptr )
{
	_stateBuffer.Pending()->AddLine( "MGDF functional test suite started" );
}

bool Module::STNew( ISimHost* host, const wchar_t *workingFolder )
{
	_workingFolder = workingFolder;
	_testModule = new Test1();

	return true;
}

bool Module::STUpdate( ISimHost* host, double elapsedTime )
{
	if ( !_testModuleCounter ) {
		host->GetTimer()->CreateCPUCounter( "Test Module", &_testModuleCounter );
	}

	if ( _testModuleCounter ) _testModuleCounter->Begin();
	_testModule->Update( host, _stateBuffer.Pending() );

	TestModule *next = _testModule->NextTestModule();
	if ( next != nullptr ) {
		delete _testModule;
		_testModule = next;
	}
	if ( _testModuleCounter ) _testModuleCounter->End();

	_stateBuffer.Flip();
	return true;
}

void Module::STShutDown( ISimHost* host )
{
	host->ShutDown();
}

bool Module::STDispose( ISimHost* host )
{
	delete this;
	return true;
}

bool Module::RTBeforeFirstDraw( MGDF::IRenderHost *host )
{
	_textManager = new TextManager( host );
	host->GetRenderTimer()->CreateGPUCounter( "Text Rendering", &_textManagerCounter );
	return true;
}

bool Module::RTDraw( IRenderHost* host, double alpha )
{
	std::shared_ptr<TextManagerState> state = _stateBuffer.Interpolate( alpha );
	if ( state ) {
		if ( _textManagerCounter ) _textManagerCounter->Begin();
		_textManager->SetState( state );
		_textManager->DrawText();
		if ( _textManagerCounter ) _textManagerCounter->End();
	}
	return true;
}

bool Module::RTBackBufferChange( IRenderHost* host )
{
	_textManager->BackBufferChange();
	return true;
}


bool Module::RTBeforeBackBufferChange( IRenderHost* host )
{
	_textManager->BeforeBackBufferChange();
	return true;
}

bool Module::RTBeforeDeviceReset( IRenderHost* host )
{
	_textManager->BeforeDeviceReset();
	return true;
}

bool Module::RTDeviceReset( IRenderHost* host )
{
	return true;
}

void Module::Panic()
{
}

}
}