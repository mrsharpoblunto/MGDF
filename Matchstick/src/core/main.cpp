#include "StdAfx.h"

#include "dbghelp.h"
#include "common/win32Exception.hpp"
#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"
#include "core.impl/MGDFSystemBuilder.hpp"
#include "MGDFApp.hpp"

void FatalErrorCallBack(std::string sender,std::string message);
void WriteMinidump();
DWORD WINAPI CrashDumpThread(LPVOID data);

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

HANDLE _dumpEvent=nullptr;
HANDLE _dumpThread=nullptr;
bool _hasDumped=false;
_MINIDUMP_EXCEPTION_INFORMATION *_dumpInfo=nullptr;
MGDFApp *_application=nullptr;

D3DAPP_WNDPROC(MGDFAppWndProc,_application)

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPreviousInstance,LPSTR lpcmdline,int nCmdShow)
{
	//Catch memory leaks
	#if defined(DEBUG) | defined(_DEBUG)
		_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	Resources::Instance(hInstance);	//initialise the core resource locator
	 
	_dumpEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    _dumpThread = CreateThread(nullptr, 0, CrashDumpThread, nullptr, 0, 0);
	SetUnhandledExceptionFilter(UnhandledExceptionCallBack);//logs and stackdumps when any unexpected errors occur

	SetErrorMode( SEM_NOGPFAULTERRORBOX );

	//create the application instance and initialise the window
	_application = new MGDFApp(hInstance);

	//create the system object and related components
	System *system = SystemBuilder::CreateSystem();
	
	GetLoggerImpl()->Add("MGDF::WinMain","starting up...");
	if (system!=nullptr) {
		system->AddFatalErrorCallback(&FatalErrorCallBack);

		_application->SetSystem(system);
		_application->InitDirect3D("MGDF",MGDFAppWndProc);

		unsigned int simulationFps = atoi(system->GetGame()->GetPreference("simFps"));
		_application->Run(simulationFps);
	}

	//dispose of the system and related components
	SystemBuilder::DisposeSystem(system);

	GetLoggerImpl()->Add("MGDF::WinMain","shutting down...");
	delete _application;

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
	
	_dumpInfo = (_MINIDUMP_EXCEPTION_INFORMATION *)malloc(sizeof(_MINIDUMP_EXCEPTION_INFORMATION));
	_dumpInfo->ThreadId = GetCurrentThreadId();
	_dumpInfo->ExceptionPointers = pExceptionInfo;
	_dumpInfo->ClientPointers = 0;

	WriteMinidump();
	
	if (!GetParameterManagerImpl()->HasParameter("hideerrors")) {
		MessageBox(nullptr,("Unexpected Win32 error\r\n\r\nFor more information, please view the log file\r\n'"+Resources::ToString(Resources::Instance().LogFile())+"'").c_str(),"FATAL ERROR",MB_OK);//output a nasty error message
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

/**
handle fatal errors explicitly invoked from the MGDF system
*/
void FatalErrorCallBack(std::string sender,std::string message) 
{
	WriteMinidump();

	if (!GetParameterManagerImpl()->HasParameter("hideerrors")) {
		MessageBox(nullptr,(message+"\r\n\r\nFor more information, please view the log file\r\n'"+Resources::ToString(Resources::Instance().LogFile())+"'").c_str(),"FATAL ERROR",MB_OK);//output a nasty error message
	}
}

void WriteMinidump()
{
	if (!_hasDumped)
	{
		_hasDumped = true;
		// MiniDumpWriteDump() doesn't write callstack for the calling thread correctly. use msdn-recommended
		// work-around of spinning a thread to do the writing
		SetEvent(_dumpEvent);
		WaitForSingleObject(_dumpThread, INFINITE);
	}
}

DWORD WINAPI CrashDumpThread(LPVOID data)
{
	WaitForSingleObject(_dumpEvent, INFINITE);

	GetLoggerImpl()->Add("MGDF::WinMain","Generating Minidump file minidump.dmp...",LOG_ERROR);
	HMODULE hDll = LoadLibraryW(L"DBGHELP.DLL");
	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			HANDLE hFile = CreateFileW((Resources::Instance().UserBaseDir()+L"minidump.dmp").c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL, NULL );

			if (hFile!=INVALID_HANDLE_VALUE)
			{
				BOOL ok = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, _dumpInfo, nullptr, nullptr );
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
	if (_dumpInfo!=nullptr) free(_dumpInfo);

	return 0;
}
