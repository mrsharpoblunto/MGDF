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
	return true;
}

bool Module::DrawScene(double alpha)
{
   return true;
}

bool Module::BackBufferChanged()
{
	return true;
}

void Module::Panic()
{
}

void Module::ShutDown()
{
}

const char * Module::GetLastError()
{
	return _lastError.c_str();
}
