/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file StructuredExceptions.h
#pragma once

#include <windows.h>

namespace WinUnitLib
{
    /// This class includes structured exception-related functionality.
    class StructuredExceptions
    {
    public:
        // For centralizing getting the strings associated with various 
        // structured exceptions.
        static const wchar_t* GetDescription(
            DWORD exceptionCode, // Structured exception code found in EXCEPTION_RECORD 
            DWORD type = 0       // Used only for access violations; 0 = read / 1 = write
        );

        // Determines whether the given structured exception is one we want to 
        // handle (vs. just crashing).
        static int Filter(LPEXCEPTION_POINTERS pExceptionInformation,
            EXCEPTION_RECORD* pExceptionRecordStorage);

    private:
        StructuredExceptions();
    };
}