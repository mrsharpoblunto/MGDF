#include "StdAfx.h"

#include "MGDFModuleFactory.hpp"

#include <boost/filesystem/operations.hpp>
#include "../common/MGDFResources.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

ModuleFactory::~ModuleFactory()
{
	if (_moduleInstance!=NULL) {
		//FreeLibrary(_moduleInstance);
	}
}

ModuleFactory::ModuleFactory(IGame *game)
{
	_game = game;

	_moduleInstance = NULL;
	_getCustomArchiveHandlers = NULL;
	_getModule = NULL;
	_isCompatibleInterfaceVersion = NULL;

	boost::filesystem::wpath globalModule(Resources::Instance().Module(),boost::filesystem::native);
	if (boost::filesystem::exists(globalModule)) 
	{
		GetLoggerImpl()->Add(THIS_NAME,"Loading Module.dll");
		CurrentDirectoryHelper::Instance().Push(Resources::Instance().BinDir());
		_moduleInstance = LoadLibraryW(Resources::Instance().Module().c_str());
		CurrentDirectoryHelper::Instance().Pop();

		if (_moduleInstance!=NULL) {
			_getCustomArchiveHandlers = (GetCustomArchiveHandlersPtr)GetProcAddress(_moduleInstance, "GetCustomArchiveHandlers");
			if (_getCustomArchiveHandlers!=NULL) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded CustomArchiveHandlers from Module.dll");
			}

			_getModule = (GetModulePtr)GetProcAddress(_moduleInstance, "GetModule");
			if (_getModule!=NULL) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded Module from Module.dll");
			}

			_isCompatibleInterfaceVersion = (IsCompatibleInterfaceVersionPtr)GetProcAddress(_moduleInstance, "IsCompatibleInterfaceVersion");
			if (_isCompatibleInterfaceVersion!=NULL) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded IsCompatibleInterfaceVersion from Module.dll");
			}
		}
		else {
			GetLoggerImpl()->Add(THIS_NAME,"Failed to load Module.dll");
		}
	}
}

ICustomArchiveHandlers *ModuleFactory::GetCustomArchiveHandlers()
{
	if (_getCustomArchiveHandlers!=NULL) {
		return _getCustomArchiveHandlers();
	}
	else {
		return NULL;
	}
}

IModule *ModuleFactory::GetModule(ISystem *system)
{
	if (_getModule!=NULL) {
		return _getModule(system);
	}
	else {
		return NULL;
	}
}

bool ModuleFactory::IsCompatibleInterfaceVersion(int interfaceVersion)
{
	if (_isCompatibleInterfaceVersion!=NULL) {
		return _isCompatibleInterfaceVersion(interfaceVersion);
	}
	else {
		return false;
	}
}

}}