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

	bool STNew( ISimHost * simHost, const wchar_t *workingFolder ) override;
	bool STDispose( ISimHost * simHost ) override;
	bool STUpdate( ISimHost * simHost, double elapsedTime ) override;
	void STShutDown( ISimHost * simHost ) override;

	bool RTBeforeFirstDraw( MGDF::IRenderHost *renderHost ) override;
	bool RTDraw( IRenderHost *renderHost, double alpha ) override;
	bool RTBeforeBackBufferChange( IRenderHost *renderHost ) override;
	bool RTBackBufferChange( IRenderHost *renderHost ) override;
	bool RTBeforeDeviceReset( IRenderHost *renderHost ) override;
	bool RTDeviceReset( IRenderHost *renderHost ) override;

	void Panic() override;
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
