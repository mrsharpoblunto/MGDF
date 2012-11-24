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

//to enable the forced unloading of libraries once they are no longer in use by the application
//leaving this commented out leaves the unloading of the dlls to the OS
//NOTE: with FREE_UNUSED_LIBRARIES enabled memory leaks in external modules are not reported correctly
//#define FREE_UNUSED_LIBRARIES

namespace MGDF { namespace core {

ModuleFactory::~ModuleFactory()
{
#ifdef FREE_UNUSED_LIBRARIES
	if (_moduleInstance!=nullptr) {
		FreeLibrary(_moduleInstance);
	}
#endif
}

ModuleFactory::ModuleFactory(IGame *game)
{
	_game = game;

	_moduleInstance = nullptr;
	_getCustomArchiveHandlers = nullptr;
	_getModule = nullptr;
	_isCompatibleInterfaceVersion = nullptr;

	boost::filesystem::wpath globalModule(Resources::Instance().Module(),boost::filesystem::native);
	if (boost::filesystem::exists(globalModule)) 
	{
		GetLoggerImpl()->Add(THIS_NAME,"Loading Module.dll");
		CurrentDirectoryHelper::Instance().Push(Resources::Instance().BinDir());
		_moduleInstance = LoadLibraryW(Resources::Instance().Module().c_str());
		CurrentDirectoryHelper::Instance().Pop();

		if (_moduleInstance!=nullptr) {
			_getCustomArchiveHandlers = (GetCustomArchiveHandlersPtr)GetProcAddress(_moduleInstance, "GetCustomArchiveHandlers");
			if (_getCustomArchiveHandlers!=nullptr) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded CustomArchiveHandlers from Module.dll");
			}

			_getModule = (GetModulePtr)GetProcAddress(_moduleInstance, "GetModule");
			if (_getModule!=nullptr) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded Module from Module.dll");
			}

			_isCompatibleInterfaceVersion = (IsCompatibleInterfaceVersionPtr)GetProcAddress(_moduleInstance, "IsCompatibleInterfaceVersion");
			if (_isCompatibleInterfaceVersion!=nullptr) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded IsCompatibleInterfaceVersion from Module.dll");
			}

			_getCompatibleFeatureLevels = (GetCompatibleFeatureLevelsPtr)GetProcAddress(_moduleInstance, "GetCompatibleFeatureLevels");
			if (_getCompatibleFeatureLevels!=nullptr) {
				GetLoggerImpl()->Add(THIS_NAME,"Loaded GetCompatibleFeatureLevels from Module.dll");
			}
		}
		else {
			GetLoggerImpl()->Add(THIS_NAME,"Failed to load Module.dll");
		}
	}
}

ICustomArchiveHandlers *ModuleFactory::GetCustomArchiveHandlers()
{
	if (_getCustomArchiveHandlers!=nullptr) {
		return _getCustomArchiveHandlers();
	}
	else {
		return nullptr;
	}
}

IModule *ModuleFactory::GetModule(ISystem *system)
{
	if (_getModule!=nullptr) {
		return _getModule(system);
	}
	else {
		return nullptr;
	}
}

bool ModuleFactory::IsCompatibleInterfaceVersion(INT32 interfaceVersion)
{
	if (_isCompatibleInterfaceVersion!=nullptr) {
		return _isCompatibleInterfaceVersion(interfaceVersion);
	}
	else {
		return false;
	}
}

UINT32 ModuleFactory::GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,UINT32 *levelSize)
{
	if (_getCompatibleFeatureLevels!=nullptr) {
		return _getCompatibleFeatureLevels(levels,levelSize);
	}
	else
	{
		return false;
	}
}


}}