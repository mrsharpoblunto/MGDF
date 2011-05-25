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

bool Module::LoadModule(const char *saveDataFolder,const char *workingFolder)
{
	_workingFolder = workingFolder;

	return true;
}

bool Module::NewModule(const char *workingFolder)
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
	return true;
}

bool Module::DrawScene(double alpha)
{
   return true;
}

bool Module::DeviceReset()
{
	return true;
}

bool Module::SetDeviceState()
{
	return true;
}

bool Module::DeviceLost()
{
	return true;
}

bool Module::CheckDeviceCaps() {
	return true;
}

bool Module::SaveModule(const char *saveDataFolder)
{
	return true;
}

void Module::Panic()
{
}

const char * Module::GetLastError()
{
	return _lastError.c_str();
}
