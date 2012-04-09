#pragma once

#include <MGDF/MGDFVersion.hpp>
#include <MGDF/MGDFSound.hpp>
#include <MGDF/MGDFSoundStream.hpp>
#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFGame.hpp>
#include <MGDF/MGDFInputManager.hpp>
#include <MGDF/MGDFList.hpp>
#include <MGDF/MGDFLogger.hpp>
#include <MGDF/MGDFModule.hpp>
#include <MGDF/MGDFParameterManager.hpp>
#include <MGDF/MGDFSystem.hpp>
#include <MGDF/MGDFCustomArchiveHandlers.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>
#include <MGDF/MGDFStatisticsManager.hpp>
#include <MGDF/MGDFGraphicsManager.hpp>
#include <MGDF/MGDFError.hpp>
#include <MGDF/MGDFErrorHandler.hpp>
#include <MGDF/MGDFTimer.hpp>

//defines the dll calling conventions used
#define MGDF_DLL_EXPORT_CALLING_CONVENTION __declspec(dllexport)
#define MGDF_DLL_IMPORT_CALLING_CONVENTION __declspec(dllimport)

// this define is set if we're currently building a core component dll that is used only within the core
// select import or export depending on it
#define MGDF_CORE_EXPORT MGDF_DLL_EXPORT_CALLING_CONVENTION
#define MGDF_CORE_IMPORT MGDF_DLL_IMPORT_CALLING_CONVENTION
#ifdef MGDF_CORE_DLL_BUILDING
	#ifdef MGDF_CORE_EXPORT
		#define MGDF_CORE_DLL MGDF_CORE_EXPORT
	#else
		#define MGDF_CORE_DLL
	#endif
#else
	#ifdef MGDF_CORE_IMPORT
		#define MGDF_CORE_DLL MGDF_CORE_IMPORT
	#else
		#define MGDF_CORE_DLL
	#endif
#endif

// this define is set if we're currently building a core common dll which is used by the core and potentially a number of core component dlls
// select import or export depending on it
#define MGDF_CORE_COMMON_EXPORT MGDF_DLL_EXPORT_CALLING_CONVENTION
#define MGDF_CORE_COMMON_IMPORT MGDF_DLL_IMPORT_CALLING_CONVENTION
#ifdef MGDF_CORE_COMMON_DLL_BUILDING
	#ifdef MGDF_CORE_COMMON_EXPORT
		#define MGDF_CORE_COMMON_DLL MGDF_CORE_COMMON_EXPORT
	#else
		#define MGDF_CORE_COMMON_DLL
	#endif
#else
	#ifdef MGDF_CORE_COMMON_IMPORT
		#define MGDF_CORE_COMMON_DLL MGDF_CORE_COMMON_IMPORT
	#else
		#define MGDF_CORE_COMMON_DLL
	#endif
#endif

//some useful macro's to make deleting pointers easier
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

//useful macro for specifying the name of the class calling the macro
#include <typeinfo>
#define THIS_NAME typeid(this).name()
#define TYPE_NAME(x) typeid(x).name()