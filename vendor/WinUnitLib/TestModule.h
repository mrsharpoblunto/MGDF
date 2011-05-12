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
///  @file TestModule.h
#pragma once

#include "TestContainer.h"
#include "StructuredExceptions.h"

#include <windows.h>

namespace WinUnitLib
{
    class Logger;

    /// This class enumerates through a list of exports of a DLL, returning the
    /// ones that are considered "valid test names" (i.e. if they start with
    /// a particular prefix).  Optionally upon construction, it can be specified
    /// to ignore the prefix and just return all the export names as-is.
    class TestModule : public TestContainer
    {
    private:
        HMODULE _hModule;           ///< The HMODULE of the PE whose exports are 
                                    ///< being enumerated

        const wchar_t* _fileName;   ///< The name of the PE file
        DWORD _next;                ///< State for the enumerator

        Logger& _logger;            ///< Logger where errors are reported and 
                                    ///< OutputDebugString{A|W} is redirected to

        size_t _testPrefixLength;   ///< Length of the test prefix (length of 
                                    ///< "TEST_", or 0 if being ignored)

        /// A pointer to the "export directory" section in the loaded module
        PIMAGE_EXPORT_DIRECTORY _pImageExportDirectory; 

        PROC _originalOutputDebugStringA; ///< Pointer to original OutputDebugStringA for unhooking purposes.
        PROC _originalOutputDebugStringW; ///< Pointer to original OutputDebugStringW for unhooking purposes.

        static Logger* _pStaticLogger; ///< for use by OutputDebugString replacement functions

    protected:
        bool _hookOutputDebugString; ///< Default is true.

    public:
        TestModule(Logger& logger, const wchar_t* fileName, 
            bool ignoreTestPrefix = false);

        ~TestModule(void);

    public:
        // The enumerator functions.
        virtual bool GetFirst(TestIdentifier& first);
        virtual bool GetNext(TestIdentifier& next);
        virtual bool ExecuteTest(const TestIdentifier& testIdentifier,
                     wchar_t* buffer, size_t cchBuffer);

    private:
        
        // The following are helper functions used to get the exports 
        // of the module.
        PIMAGE_EXPORT_DIRECTORY GetExportDirectory();
        const char* GetExportNameAt(DWORD i);

        // Just checks if the export is prefixed with TEST_*, if applicable.
        bool IsValidTestFunction(const char* testFunction);

        // We want to hook OutputDebugString on the loaded module so we have
        // control of all trace output (and unhook when we're done).
        void HookOutputDebugString();
        void UnhookOutputDebugString();

        // These are the functions that the ODS* functions get temporarily
        // replaced by.
        static VOID WINAPI OutputDebugStringAReplacement(const char* message);
        static VOID WINAPI OutputDebugStringWReplacement(const wchar_t* message);
    private:
        __declspec( property( get=GetModule ) ) 
        const HMODULE Module;
        const HMODULE GetModule();

    private:
        /// This is just here (not implemented) to indicate there's currently no
        /// operator= for this class.
        TestModule& operator=(const TestModule);
   };
}