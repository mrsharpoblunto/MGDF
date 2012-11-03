#include "StdAfx.h"

#include "Module.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

Module::~Module(void)
{
}

Module::Module(MGDF::ISystem *system)
{
	_system = system;
}

bool Module::New(const wchar_t *workingFolder)
{
	_workingFolder = workingFolder;

	return true;
}

bool Module::Dispose(void)
{
	delete this;
	return true;
}

bool Module::UpdateScene(double elapsedTime)
{
	//Game logic goes here
	return true;
}

bool Module::DrawScene(double alpha)
{
	//Any rendering goes here. Note this runs in a separate thread to UpdateScene
   return true;
}

bool Module::BackBufferChanged()
{
	//Any D3D resources that may need to be updated as a result of the backbuffer
	//changing should be handled here.
	return true;
}

void Module::Panic()
{
}

void Module::ShutDown()
{
	//The system has requested that the module shutdown. Calling _system->ShutDown()
	//informs the system that the module is ready to shutdown.
	_system->ShutDown();
}

const char * Module::GetLastError()
{
	return _lastError.c_str();
}
