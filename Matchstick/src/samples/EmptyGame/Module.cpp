#include "StdAfx.h"

#include "Module.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

Module::~Module( void )
{
}

Module::Module()
{
}

bool Module::STNew( MGDF::ISimHost *host, const wchar_t *workingFolder )
{
	_workingFolder = workingFolder;

	return true;
}

bool Module::STUpdate( MGDF::ISimHost *host, double elapsedTime )
{
	//Game logic goes here
	return true;
}

void Module::STShutDown( MGDF::ISimHost *host )
{
	//The  has requested that the module shutdown. Calling host->ShutDown()
	//informs the host that the module is ready to shutdown.
	host->ShutDown();
}

bool Module::STDispose( MGDF::ISimHost *host  )
{
	delete this;
	return true;
}

bool Module::RTBeforeFirstDraw( MGDF::IRenderHost *host )
{
	// Any first time rendering initialization stuff should go here
	return true;
}

bool Module::RTDraw( MGDF::IRenderHost *host, double alpha )
{
	//Any rendering goes here.
	return true;
}

bool Module::RTBeforeBackBufferChange( MGDF::IRenderHost *host )
{
	//Anything holding a reference to the backbuffer should release it now
	//otherwise D3D will not be able to change the backbuffer
	return true;
}

bool Module::RTBackBufferChange( MGDF::IRenderHost *host )
{
	//Any D3D resources that may need to be updated as a result of the backbuffer
	//changing should be handled here.
	return true;
}

bool Module::RTBeforeDeviceReset( MGDF::IRenderHost *host )
{
	//Anything holding a reference to ANY device dependent resource should release it now
	//as these references to the removed device are now invalid
	return true;
}

bool Module::RTDeviceReset( MGDF::IRenderHost *host )
{
	//recreate any resources cleared out in RTBeforeDeviceReset
	return true;
}

void Module::Panic()
{
}
