#pragma once

#include <time.h>
#include <MGDF/MGDF.hpp>

#include "BufferedGameState.hpp"
#include "TextManager.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

namespace MGDF
{
namespace Test
{

class TestModule
{
public:
	virtual ~TestModule( void ) {}

	virtual void Update( ISimHost *host, TextManagerState *state ) = 0;

	virtual TestModule *NextTestModule() = 0;
};

class Module: public IModule
{
public:
	virtual ~Module( void );
	Module();

	bool STNew( ISimHost * simHost, const wchar_t *workingFolder ) override final;
	bool STDispose( ISimHost * simHost ) override final;
	bool STUpdate( ISimHost * simHost, double elapsedTime ) override final;
	void STShutDown( ISimHost * simHost ) override final;

	bool RTBeforeFirstDraw( MGDF::IRenderHost *renderHost ) override final;
	bool RTDraw( IRenderHost *renderHost, double alpha ) override final;
	bool RTBeforeBackBufferChange( IRenderHost *renderHost ) override final;
	bool RTBackBufferChange( IRenderHost *renderHost ) override final;
	bool RTBeforeDeviceReset( IRenderHost *renderHost ) override final;
	bool RTDeviceReset( IRenderHost *renderHost ) override final;

	void Panic() override final;
private:
	std::wstring _workingFolder;

	TestModule *_testModule;
	BufferedGameState<TextManagerState> _stateBuffer;
	TextManager *_textManager;
	MGDF::IPerformanceCounter *_textManagerCounter;
	MGDF::IPerformanceCounter *_testModuleCounter;
};

}
}
