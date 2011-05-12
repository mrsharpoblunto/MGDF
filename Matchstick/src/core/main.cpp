#include "StdAfx.h"

#include "dbghelp.h"
#include "common/win32Exception.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"
#include "core.impl/MGDFSystemBuilder.hpp"
#include "MGDFApp.hpp"

void FatalErrorCallBack(std::string sender,std::string message);

LONG WINAPI UnhandledExceptionCallBack( struct _EXCEPTION_POINTERS *pExceptionInfo );
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
									CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
									CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
									CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
									);
using namespace MGDF;
using namespace MGDF::core;

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

D3DAPP_WNDPROC(MGDFAppWndProc,MGDFApp::InstancePtr())

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPreviousInstance,LPSTR lpcmdline,int nCmdShow)
{
	//Catch memory leaks
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	Resources::Instance(hInstance);	//initialise the core resource locator
	GetLoggerImpl()->Add("MGDF::WinMain","starting up...");
	SetUnhandledExceptionFilter(UnhandledExceptionCallBack);//logs and stackdumps when any unexpected errors occur

	//create the application instance and initialise the window
	new MGDFApp(hInstance);

	//create the system object and related components
	System *system = SystemBuilder::CreateSystem(MGDFApp::Instance().GetApplicationInstance(),MGDFApp::Instance().GetWindow());

	if (system!=NULL) {
		system->AddFatalErrorCallback(&FatalErrorCallBack);

		MGDFApp::Instance().SetSystem(system);
		MGDFApp::Instance().InitDirect3D("MGDF",MGDFAppWndProc,D3DDEVTYPE_HAL,D3DCREATE_HARDWARE_VERTEXPROCESSING);

		unsigned int simulationFps = atoi(system->GetGame()->GetPreference("simFps"));
		MGDFApp::Instance().Run(simulationFps);
	}

	GetLoggerImpl()->Add("MGDF::WinMain","shutting down...");
	delete MGDFApp::InstancePtr();

	//dispose of the system and related components
	SystemBuilder::DisposeSystem(system);

	GetLoggerImpl()->Add("MGDF::WinMain","shut down successfully");
	return 0;
}

/**
log & handle unexpected win32 errors before the exception is thrown
*/
LONG WINAPI UnhandledExceptionCallBack( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	GetLoggerImpl()->Add("MGDF::WinMain","WIN32 ERROR: "+Win32Exception::TranslateError(pExceptionInfo->ExceptionRecord->ExceptionCode),LOG_ERROR);
	GetLoggerImpl()->Add("MGDF::WinMain","Generating Minidump file minidump.dmp...",LOG_ERROR);
	HMODULE hDll = LoadLibrary( "DBGHELP.DLL" );
	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			HANDLE hFile = CreateFile((Resources::Instance().UserBaseDir()+"minidump.dmp").c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL, NULL );

			if (hFile!=INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
				ExInfo.ThreadId = GetCurrentThreadId();
				ExInfo.ExceptionPointers = pExceptionInfo;
				ExInfo.ClientPointers = NULL;

				BOOL ok = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
				if (!ok)
				{
					GetLoggerImpl()->Add("MGDF::WinMain","Failed to save dump file");
				}
				CloseHandle(hFile);
			}
			else
			{
				GetLoggerImpl()->Add("MGDF::WinMain","Failed to save dump file");
			}
		}
	}

	GetLoggerImpl()->Flush();
	
	if (!GetParameterManagerImpl()->HasParameter("hideerrors")) {
		MessageBox(NULL,("Unexpected Win32 error\r\n\r\nFor more information, please view the log file\r\n'"+Resources::Instance().LogFile()+"'").c_str(),"FATAL ERROR",MB_OK);//output a nasty error message
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

/**
handle fatal errors explicitly invoked from the MGDF system
*/
void FatalErrorCallBack(std::string sender,std::string message) 
{
	GetLoggerImpl()->Add("MGDF::WinMain","Generating Minidump file minidump.dmp...",LOG_ERROR);
	HMODULE hDll = LoadLibrary( "DBGHELP.DLL" );
	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			HANDLE hFile = CreateFile((Resources::Instance().UserBaseDir()+"minidump.dmp").c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL, NULL );

			if (hFile!=INVALID_HANDLE_VALUE)
			{
				BOOL ok = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, NULL, NULL, NULL );
				if (!ok)
				{
					GetLoggerImpl()->Add("MGDF::WinMain","Failed to save dump file");
				}
				CloseHandle(hFile);
			}
			else
			{
				GetLoggerImpl()->Add("MGDF::WinMain","Failed to save dump file");
			}
		}
	}
	GetLoggerImpl()->Flush();

	if (!GetParameterManagerImpl()->HasParameter("hideerrors")) {
		MessageBox(NULL,(message+"\r\n\r\nFor more information, please view the log file\r\n'"+Resources::Instance().LogFile()+"'").c_str(),"FATAL ERROR",MB_OK);//output a nasty error message
	}
}