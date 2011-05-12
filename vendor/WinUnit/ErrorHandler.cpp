/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */

/**
 *  @file ErrorHandler.cpp
 *  The implementation file for the application-wide error handling functions
 *  used by WinUnit.exe.  These have been put in a separate class to avoid
 *  cluttering up main (and so the dependencies can be more easily removed
 *  if desired).
 */

#include "ErrorHandler.h"

#include "ReturnValues.h"
#include "Logger.h"
#include "StructuredExceptions.h"

#include <windows.h> // for SetUnhandledExceptionFilter
#include <eh.h>      // for set_terminate
#include <crtdbg.h>  // for _CrtSetReport*
#include <signal.h>  // for signal()

#include <stdio.h>

using namespace WinUnitLib;

/// This function sets up a process-wide unhandled exception handler.
void ErrorHandler::Initialize()
{
    ::SetUnhandledExceptionFilter(UnhandledExceptionFilter);
}

/// This function is intended to disable error dialogs as well as possible
/// to make automating running the tool easier. It is called when the -n option
/// is specified.
/// @remarks
/// The only settings that are per-process are SetErrorMode and 
/// SetUnhandledExceptionFilter (the latter being called in Initialize).
/// The rest are per-CRT.  This means that they will only apply to errors in 
/// executables outside this one if the external executable is using the same
/// CRT.  This executable (WinUnit.exe) is compiled with the /MDd switch for
/// Debug and the /MD switch for Release.  Test code called by this executable
/// needs to have been compiled with the same switches and using the same CRT
/// DLL in order for the CRT-specific settings to be effective.  Fortunately
/// the errors handled by these settings are uncommon and do indicate serious 
/// problems that should be fixed.  When they are fixed, trapping them will
/// no longer be a problem!
void ErrorHandler::DisableErrorDialogs()
{
    s_nonInteractive = true;

    /* DWORD oldProcessErrorMode = */ ::SetErrorMode(SEM_NOGPFAULTERRORBOX);

    // Send debug-mode error messages to STDERR (no dialog box)
    /* int oldErrorReportMode = */
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    /* _HFILE oldErrorFile = */ 
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);

    // Send assert messages to the debug to STDERR (no dialog box)
    /* int oldAssertReportMode = */
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    /* _HFILE oldAssertFile = */ 
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);

    // Called instead of terminate()
    /* terminate_function prevTerminateFunction = */ 
        ::set_terminate(TerminateFunction);

    // Called when abort() is called
    /* SignalHandlerPointer previousHandler = */
        signal(SIGABRT, AbortFunction);

    // Suppresses the usual abort warning when abort() is called
    /* unsigned int oldAbortFlags = */
        ::_set_abort_behavior(0, _WRITE_ABORT_MSG);

    // Send messages from "assert" to stderr rather than bringing up a dialog.
    /* int oldCrtErrorMode = */ 
        ::_set_error_mode(_OUT_TO_STDERR);
}

/// Sets the static logger variable so the error messages have somewhere to
/// go (if it's not set, they go to stderr).
/// @param pLogger  The Logger error messages are to be sent to.
void ErrorHandler::SetLogger(Logger* pLogger)
{
    s_pLogger = pLogger;
}


/// This function deals with the structured exceptions that aren't caught 
/// elsewhere.  It displays an appropriate error message to the logger
/// (if set) and then either calls ExitProcess (if "no UI" was specified)
/// or continues on to the REAL UnhandledExceptionFilter, which shows 
/// the customary crash dialog.
/// @return EXCEPTION_CONTINUE_SEARCH if we want to continue on and show
/// the default crash dialog.
LONG WINAPI ErrorHandler::UnhandledExceptionFilter(
    EXCEPTION_POINTERS* pExceptionPointers) ///< Pointer to information about the exception
{
    const wchar_t* exceptionMessage = L"";
    if (pExceptionPointers != NULL && pExceptionPointers->ExceptionRecord != NULL)
    {
        DWORD exceptionCode = pExceptionPointers->ExceptionRecord->ExceptionCode;
        ULONG extraInformation = 
           (exceptionCode == EXCEPTION_ACCESS_VIOLATION ?
            (DWORD)pExceptionPointers->ExceptionRecord->ExceptionInformation[0] :
            0);

        exceptionMessage = StructuredExceptions::GetDescription(
            exceptionCode, extraInformation);
    }

    DisplayError(L"Premature shutdown. An unhandled exception occurred.", 
        exceptionMessage);
        
    if (s_nonInteractive)
    {
        ::ExitProcess(WINUNIT_EXIT_UNHANDLED_EXCEPTION);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

/// If ErrorHandler::DisableErrorDialogs was called (meaning the -n option
/// was specified), this function replaces calls to terminate().
/// @remarks
/// According to the documentation, terminate() is called in the following 
/// cases:
/// - A matching catch handler cannot be found for a thrown C++ exception.
/// - An exception is thrown by a destructor function during stack unwind.
/// - The stack is corrupted after throwing an exception.
void ErrorHandler::TerminateFunction()
{
    DisplayError(L"Premature shutdown.  terminate() was called.");
    ::ExitProcess(WINUNIT_EXIT_UNHANDLED_EXCEPTION);
}

/// If ErrorHandler::DisableErrorDialogs was called (meaning the -n option
/// was specified), this function is called when abort() is called.
void ErrorHandler::AbortFunction(int /* signal */)
{
    DisplayError(L"Premature shutdown.  abort() was called.");
    ::ExitProcess(WINUNIT_EXIT_UNHANDLED_EXCEPTION);
}

/// This function displays error messages in a standard way, called by the
/// the three error handlers above.
/// @remarks
/// I'm taking the risk of using non-trivial functions and the CRT here but, 
/// well, the app was going to shut down anyway.
void ErrorHandler::DisplayError(const wchar_t* errorMessage,        
                                const wchar_t* details /* L"" */)
{
    if (s_pLogger)
    {
        s_pLogger->OutputProgramErrorLine(errorMessage, L" ", details);
        s_pLogger = NULL;
    }
    else
    {
        ::fwprintf(stderr, L"[stderr] %s %s\n", errorMessage, details);
    }
}

Logger* ErrorHandler::s_pLogger = NULL;
bool ErrorHandler::s_nonInteractive = false;
