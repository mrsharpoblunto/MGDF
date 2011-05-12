#pragma once

//defines the dll calling conventions used
#define TESTCOMMON_DLL_EXPORT_CALLING_CONVENTION __declspec(dllexport)
#define TESTCOMMON_DLL_IMPORT_CALLING_CONVENTION __declspec(dllimport)

// this define is set if we're currently building a core component dll that is used only within the core
// select import or export depending on it
#define TESTCOMMON_EXPORT TESTCOMMON_DLL_EXPORT_CALLING_CONVENTION
#define TESTCOMMON_IMPORT TESTCOMMON_DLL_IMPORT_CALLING_CONVENTION
#ifdef TESTCOMMON_DLL_BUILDING
	#ifdef TESTCOMMON_EXPORT
		#define TESTCOMMON_DLL TESTCOMMON_EXPORT
	#else
		#define TESTCOMMON_DLL
	#endif
#else
	#ifdef TESTCOMMON_IMPORT
		#define TESTCOMMON_DLL TESTCOMMON_IMPORT
	#else
		#define TESTCOMMON_DLL
	#endif
#endif