/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file Logger.cpp

#include "Logger.h"

#include <stdio.h>
#include <windows.h>

namespace WinUnitLib
{

    Logger::Logger(void)
    :
    _next(NULL)
    {
    }

    void Logger::SetNext(Logger* next)
    {
        _next = next;
    }

    Logger* Logger::GetNext()
    {
        return _next;
    }

    void Logger::OutputWinapiError(
        DWORD errorCode,                ///< Error code from GetLastError().
        wchar_t* callingFunction,       ///< Name of calling function.
        const wchar_t* msg1,            ///< Additional information from caller.
        const wchar_t* msg2 /* NULL */) ///< Optional additional information from caller.
    {
        wchar_t buffer[80] = L"";

        DWORD charsWritten = FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            errorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            buffer,
            ARRAYSIZE(buffer), NULL 
        );

        // Remove trailing \r\n
        if (charsWritten >= 2 && 
            buffer[charsWritten - 1] == L'\n' &&
            buffer[charsWritten - 2] == L'\r')
        {
            buffer[charsWritten - 2] = L'\0';
        }

        wchar_t errorMessage[1024] = L"";
        /* int cchWritten = */ ::_snwprintf_s(errorMessage, ARRAYSIZE(errorMessage), _TRUNCATE,
            L"%s: %s%s. %s", callingFunction, 
            msg1,
            (msg2 != NULL) ? msg2 : L"",
            (charsWritten > 0) ? buffer : L"");

        OutputProgramErrorLine(errorMessage);
    }
}