#include "StdAfx.h"

#include <Dbghelp.h>
#include <filesystem>

#include "MGDFModuleFactory.hpp"

#include "../common/MGDFResources.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFCurrentDirectoryHelper.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

//to enable the forced unloading of libraries once they are no longer in use by the application
//leaving this commented out leaves the unloading of the dlls to the OS
//NOTE: with FREE_UNUSED_LIBRARIES enabled memory leaks in external modules are not reported correctly
//#define FREE_UNUSED_LIBRARIES

using namespace std::tr2::sys;

namespace MGDF
{
namespace core
{

ModuleFactory::~ModuleFactory()
{
#ifdef FREE_UNUSED_LIBRARIES
	if ( _moduleInstance != nullptr ) {
		FreeLibrary( _moduleInstance );
	}
#endif
}

MGDFError ModuleFactory::TryCreate( ModuleFactory **factory )
{
	*factory = new ModuleFactory();
	MGDFError error = (*factory)->Init();
	if ( MGDF_OK != error ) {
		delete *factory;
		*factory = nullptr;
	}
	return error;
}

ModuleFactory::ModuleFactory()
	: _moduleInstance( nullptr )
	, _getCustomArchiveHandlers( nullptr )
	, _getModule( nullptr )
	, _isCompatibleInterfaceVersion( nullptr )
	, _getCompatibleFeatureLevels( nullptr )
{
}

MGDFError ModuleFactory::Init()
{
	wpath globalModule( Resources::Instance().Module() );
	if ( exists( globalModule ) ) {
		LOG( "Loading Module.dll", LOG_LOW );
		CurrentDirectoryHelper::Instance().Push( Resources::Instance().BinDir() );
		_moduleInstance = LoadLibraryW( Resources::Instance().Module().c_str() );
		CurrentDirectoryHelper::Instance().Pop();

		if ( _moduleInstance != nullptr ) {
			// required exported functions
			_getModule = ( GetModulePtr ) GetProcAddress( _moduleInstance, "GetModule" );
			if ( _getModule != nullptr ) {
				LOG( "Loaded GetModule from Module.dll", LOG_LOW );
			} else {
				LOG( "Module has no exported GetModule function", LOG_ERROR );
				return MGDF_ERR_FATAL;
			}

			_isCompatibleInterfaceVersion = ( IsCompatibleInterfaceVersionPtr ) GetProcAddress( _moduleInstance, "IsCompatibleInterfaceVersion" );
			if ( _isCompatibleInterfaceVersion != nullptr ) {
				LOG( "Loaded IsCompatibleInterfaceVersion from Module.dll", LOG_LOW );
			} else {
				LOG( "Module has no exported IsCompatibleInterfaceVersion function", LOG_ERROR );
				return MGDF_ERR_FATAL;
			}

			// optional exported functions
			_getCustomArchiveHandlers = ( GetCustomArchiveHandlersPtr ) GetProcAddress( _moduleInstance, "GetCustomArchiveHandlers" );
			if ( _getCustomArchiveHandlers != nullptr ) {
				LOG( "Loaded GetCustomArchiveHandlers from Module.dll", LOG_LOW );
			} else {
				LOG( "Module has no exported GetCustomArchiveHandlers function", LOG_LOW );
			}

			_getCompatibleFeatureLevels = ( GetCompatibleFeatureLevelsPtr ) GetProcAddress( _moduleInstance, "GetCompatibleFeatureLevels" );
			if ( _getCompatibleFeatureLevels != nullptr ) {
				LOG( "Loaded GetCompatibleFeatureLevels from Module.dll", LOG_LOW );
			} else {
				LOG( "Module has no exported GetCompatibleFeatureLevels function", LOG_LOW );
			}

			return MGDF_OK;
		} else {
			LOG( "Failed to load Module.dll", LOG_ERROR );

#if defined (_WIN64) 
			bool win64 = true;	
#else
			bool win64 = false;
#endif
			bool loggedMessage = false;

			HANDLE file = CreateFileW( Resources::Instance().Module().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
			if (file == INVALID_HANDLE_VALUE) goto cleanup;

			HANDLE fileMapping = CreateFileMapping(file, NULL, PAGE_READONLY | SEC_IMAGE, 0, 0, NULL );
			if (fileMapping == INVALID_HANDLE_VALUE) goto cleanup;

			LPVOID addressHeader = MapViewOfFileEx(fileMapping, FILE_MAP_READ, 0, 0, 0, NULL);
			if (addressHeader == NULL) goto cleanup; //couldn't memory map the file

			PIMAGE_NT_HEADERS peHeader = ImageNtHeader(addressHeader);
			if (peHeader == NULL) goto cleanup; //couldn't read the header

			if (peHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386 && win64)
			{
				LOG( "Failed to load Module.dll - MGDF core is 64 bit and Module is 32 bit", LOG_ERROR );
				loggedMessage = true;
			}
			else if (peHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 && !win64)
			{
				LOG( "Failed to load Module.dll - MGDF core is 32 bit and Module is 64 bit", LOG_ERROR );
				loggedMessage = true;		
			}
cleanup:
			if (!loggedMessage) {
				LOG( "Failed to load Module.dll - It doesn't appear to be a valid dll", LOG_ERROR );
			}
			if (file != INVALID_HANDLE_VALUE) {
				CloseHandle(file);
			}
			if (fileMapping != INVALID_HANDLE_VALUE) {
				CloseHandle(fileMapping);
			}
		}
	}
	return MGDF_ERR_FATAL;
}



bool ModuleFactory::GetCustomArchiveHandlers( IArchiveHandler **list, UINT32 *length, ILogger *logger, IErrorHandler *errorHandler ) const
{
	if ( _getCustomArchiveHandlers != nullptr ) {
		return _getCustomArchiveHandlers( list, length, logger, errorHandler );
	} else {
		return true;
	}
}

IModule *ModuleFactory::GetModule() const
{
	if ( _getModule != nullptr ) {
		return _getModule();
	} else {
		return nullptr;
	}
}

bool ModuleFactory::IsCompatibleInterfaceVersion( INT32 interfaceVersion ) const
{
	if ( _isCompatibleInterfaceVersion != nullptr ) {
		return _isCompatibleInterfaceVersion( interfaceVersion );
	} else {
		return false;
	}
}

UINT32 ModuleFactory::GetCompatibleFeatureLevels( D3D_FEATURE_LEVEL *levels, UINT32 *levelSize ) const
{
	if ( _getCompatibleFeatureLevels != nullptr ) {
		return _getCompatibleFeatureLevels( levels, levelSize );
	} else {
		return 0;
	}
}

bool ModuleFactory::GetLastError( std::string& error ) const
{
	error = _lastError;
	return _lastError.length()>0;
}


}
}