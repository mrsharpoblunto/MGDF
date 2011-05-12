/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)

   The functionality in this class was based on code and explanation found in
   Debugging Applications for Microsoft .NET and Microsoft Windows (2003) 
   by John Robbins, and the Inside Windows columns for MSDN Magazine by Matt
   Pietrek, February and March 2002.  
   http://msdn.microsoft.com/msdnmag/issues/02/02/PE/default.aspx
   http://msdn.microsoft.com/msdnmag/issues/02/03/PE2/default.aspx
   (Any errors in style or functionality are mine. -Maria)
 * -------------------------------------------------------------------------- */
/// @file ImportHooker.h
#pragma once

#include <windows.h>

namespace WinUnitLib
{
    class Logger;

    /// This class contains static functions required for hooking imports.
    class ImportHooker
    {
    public:
        
        /// Contains the name of a function to be hooked and a procedure to replace it with.
        struct HOOKFUNCDESC
        {
            /// The name of the function to hook.
            LPCSTR szFunc;
            /// The procedure to replace it with.
            PROC pProc;
        };

    public:

        // Replaces calls to specified imports with alternative procs.
        static BOOL HookImportedFunctionsByName(
            HMODULE hModule,
            LPCSTR szImportMod,
            UINT uiCount,
            const HOOKFUNCDESC* paHookArray,
            PROC* paOrigFuncs,
            DWORD* pdwHooked,
            Logger& logger);
    private:
        static PIMAGE_IMPORT_DESCRIPTOR GetNamedImportDescriptor(
            HMODULE hModule, LPCSTR szImportMod, Logger& logger);

    private:
        // Constructor/destructor are private as this class only has static
        // members.
        ImportHooker(void);
        ~ImportHooker(void);
    };
}