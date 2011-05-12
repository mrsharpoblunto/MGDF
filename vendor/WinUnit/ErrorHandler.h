/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */

/**
 *  @file ErrorHandler.h
 *  The header file for the application-wide error handling functions used by
 *  WinUnit.exe.
 */

#pragma once

#include <windows.h>

namespace WinUnitLib
{
    class Logger;
}

/// This class contains static functions used for system-wide error handling.
class ErrorHandler
{
    typedef void (*SignalHandlerPointer)(int);

public:
    // Sets up application-wide exception handling.
    static void Initialize();

    // Sets process- and CRT-wide variables that disable dialogs for several
    // classes of errors and asserts.
    static void DisableErrorDialogs();

    // Sets a logger variable for error functions to write to.
    static void SetLogger(WinUnitLib::Logger* pLogger);

private:
    /// A static pointer to the Logger is required for use by
    /// the unhandled exception filter.
    static WinUnitLib::Logger* s_pLogger;

    /// This is for use by the unhandled exception filter--if false, the filter
    /// allows the crash dialog to go up after printing an error message.
    static bool s_nonInteractive;

    // This is the function that gets called when an unhandled exception
    // bubbles up to the top.
    static LONG WINAPI ErrorHandler::UnhandledExceptionFilter(
        EXCEPTION_POINTERS* pExceptionPointers);

    // The function that replaces terminate().
    static void TerminateFunction();

    // The function that is called when abort() is called.
    static void AbortFunction(int /* signal */);

    // Called by the other error handlers to display the error message.
    static void DisplayError(const wchar_t* errorMessage, 
        const wchar_t* details = L"");

private:
    ~ErrorHandler(void);
};
