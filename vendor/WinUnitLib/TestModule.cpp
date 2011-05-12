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
/// @file TestModule.cpp

#include "TestModule.h"
#include "TestIdentifier.h"
#include "Logger.h"
#include "TestConstants.h"
#include "ImportHooker.h"

#include <stdio.h>

// RVAs are represented as DWORDs.  This makes it easier to add them together
// to use as a regular pointer.
#define MakePtr(cast, ptr, addValue) (cast)((DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))

using namespace TestConstants;

namespace WinUnitLib
{
#pragma region Constructor/destructor

    /// Constructor initializes member variables.
    TestModule::TestModule(
        Logger& logger,             ///< Used by OutputDebugString replacements 
                                    ///< and to write program output to 
        const wchar_t* fileName,    ///< The filename whose exports we're enumerating
        bool ignoreTestPrefix /* false */) ///< True if we should ignore whether
                                           ///< an export starts with "TEST_"
                                           ///< and just enumerate all of them
    :
    _fileName(fileName),
    _hModule(0),
    _next((DWORD)-1),
    _logger(logger),
    _pImageExportDirectory(NULL),
    _originalOutputDebugStringA(NULL),
    _originalOutputDebugStringW(NULL),
    _hookOutputDebugString(true)
    {
        // If we want to ignore the test prefix, we pretend its length is 0.
        _testPrefixLength = (ignoreTestPrefix ? 0 : strlen(TestPrefix));
    }

    /// Destructor unloads the library.
    TestModule::~TestModule(void)
    {
        // free the library
        if (_hModule != NULL)
        {
            if (_hookOutputDebugString)
            {
                this->UnhookOutputDebugString();
            }
            BOOL freed = ::FreeLibrary(_hModule);
            if (!freed)
            {
                _logger.OutputWinapiError(GetLastError(), __WFUNCTION__,
                    L"FreeLibrary failed for ",
                    this->_fileName);
            }
        }
    }
#pragma endregion

#pragma region Public methods

    /// Gets the first TestIdentifier among the list of tests being enumerated
    /// (i.e. the valid TEST_* exports of the given DLL).
    /// @return true if retrieval of the first test was successful; false otherwise.
    /// @remarks
    /// Use the returned TestIdentifier immediately or copy its values, 
    /// because it points directly into the loaded module's address space,
    /// and therefore will not be valid when the module is unloaded.
    bool TestModule::GetFirst(
        TestIdentifier& testIdentifier) ///< [out] Reference to the first valid TestIdentifier.
    {
        _next = 0; // Calling GetFirst twice is allowed.

        // get the first item in the exports directory
        return this->GetNext(testIdentifier);
    }

    /// Gets the next valid TestIdentifier among those being enumerated.  If 
    /// none, returns false, and the passed-in reference is left untouched.
    /// @return true if retrieval was successful; false otherwise.
    /// @remarks
    /// Use the returned TestIdentifier immediately or copy its values, 
    /// because it points directly into the loaded module's address space,
    /// and therefore will not be valid when the module is unloaded.
    bool TestModule::GetNext(
        TestIdentifier& testIdentifier) ///< [out] Reference to the next valid TestIdentifier.
    {
        // GetFirst must be called first.
        if (_next == (DWORD)-1)
        {
            _logger.OutputProgramErrorLine(__WFUNCTION__ L": GetFirst must be called first.");
            return false;
        }

        // get the current item in the exports directory; check if it's
        // satisfactory as a test.  If not, continue until we hit one that is
        // or the end of the list.
        
        const char* exportName = GetExportNameAt(_next++);
        while(exportName != NULL &&
              !IsValidTestFunction(exportName))
        {
            exportName = GetExportNameAt(_next++);
        }

        if (exportName == NULL) { return false; }

        testIdentifier = 
            TestIdentifier(this->_hModule, exportName, exportName + _testPrefixLength);

        return true;
    }

    /// Runs a single test, placing any error output in the buffer.
    /// @return true if test succeeded; otherwise false
    /// @remarks 
    /// If test passed, buffer will not be checked.
    bool TestModule::ExecuteTest(
       const TestIdentifier& testIdentifier,    ///< Test to be run.
       wchar_t* buffer,                         ///< Buffer to receive error output.
       size_t cchBuffer)                        ///< Size of buffer (in wide-chars).
    {
        // Initialize buffer.
        if (cchBuffer > 0) { buffer[0] = L'\0'; }

        HMODULE hModule = this->Module;

        // If container has not been initialized, fail.
        if (hModule == NULL)
        {
            ::_snwprintf_s(buffer, cchBuffer, _TRUNCATE, 
                L"ERROR: %S: Test module %s failed to load. ",
                __FUNCTION__, this->_fileName);
            return false;
        }

        // If we've been given a wrong test, fail.
        if (testIdentifier.Module != hModule)
        {
            ::_snwprintf_s(buffer, cchBuffer, _TRUNCATE, 
                L"ERROR: %S: Unrecognized test identifier (wrong HMODULE). ", __FUNCTION__);
            return false;
        }

        TestPtr testPtr = (TestPtr)::GetProcAddress(
            hModule, testIdentifier.OriginalExportName);

        if (testPtr == NULL) 
        { 
            ::_snwprintf_s(buffer, cchBuffer, _TRUNCATE, 
                L"ERROR: %S: Unable to load test function %S in file %s. ",  
                __FUNCTION__, testIdentifier.OriginalExportName, _fileName);
            return false; 
        }

        // Execute the function
        return (*testPtr)(buffer, cchBuffer);
    }


#pragma endregion

#pragma region Helper functions

    /// Gets the "PIMAGE_EXPORT_DIRECTORY" in this executable, if any (the section
    /// which includes the names of the exports).
    /// @return A pointer to the image export directory, if retrieved; otherwise NULL.
    /// @remarks 
    /// This is called everywhere the pImageExportDirectory is needed, so it is
    /// cached on first load.
    PIMAGE_EXPORT_DIRECTORY TestModule::GetExportDirectory()
    {
        if (this->_pImageExportDirectory == NULL)
        {
            HMODULE hModule = this->Module;
            if (hModule == NULL) { return NULL; }

            PIMAGE_DOS_HEADER pDOSHeader = (PIMAGE_DOS_HEADER)hModule ;

            // Is this the MZ header?
            if (IsBadReadPtr(pDOSHeader, sizeof(IMAGE_DOS_HEADER)) ||
                (IMAGE_DOS_SIGNATURE != pDOSHeader->e_magic))
            {
                _logger.OutputProgramErrorLine(__WFUNCTION__ L": Invalid PE.");
                return NULL;
            }

            // Get the PE header.
            PIMAGE_NT_HEADERS pNTHeader = MakePtr(PIMAGE_NT_HEADERS,
                                                  pDOSHeader,
                                                  pDOSHeader->e_lfanew);

            // Is this a real PE image?
            if (IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)) ||
                (IMAGE_NT_SIGNATURE != pNTHeader->Signature))
            {
                _logger.OutputProgramErrorLine(__WFUNCTION__ L": Invalid PE signature.");
                return NULL;
            }

            // If there is no exports section, leave now.
            if ( 0 == pNTHeader->OptionalHeader.
                                 DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].
                                                              VirtualAddress)
            {
                _logger.OutputProgramErrorLine(__WFUNCTION__ L": No exports found.");
                return NULL;
            }

            // Get the pointer to the exports section.
            _pImageExportDirectory
                = MakePtr (PIMAGE_EXPORT_DIRECTORY,
                           pDOSHeader,
                           pNTHeader->OptionalHeader.
                                 DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].
                                                              VirtualAddress);

        }
        return _pImageExportDirectory;
    }

    /// Get an export name from the export directory.
    /// @return A pointer to the export found at index i, or NULL if there is none.
    const char* TestModule::GetExportNameAt(
        DWORD i) ///< The index at which the export is to be found.
    {
        const HMODULE hModule = this->Module;
        if (hModule == NULL) { return NULL; }

        PIMAGE_EXPORT_DIRECTORY pImageExportDirectory = this->GetExportDirectory();
        if (pImageExportDirectory == NULL) { return NULL; }

        // Used to get relative addresses (everything is stored as a DWORD which
        // needs to be appended to the image base in order to get the actual address)
        PBYTE pImageBase = (PBYTE)hModule;

        // Get the array of relative pointers to all the function names
        DWORD* pszFuncNames = MakePtr(DWORD*, pImageBase, pImageExportDirectory->AddressOfNames);

        if (i >= pImageExportDirectory->NumberOfNames) { return NULL; }

        const char* exportName = MakePtr(const char*, pImageBase, pszFuncNames[i]);

        return exportName;
    }

    /// Indicate whether a particular name of a function is valid as a 
    /// test function (must start with the test prefix, if ignoreTestPrefix
    /// is not set).
    /// @return true if function name is a valid test name; otherwise false.
    bool TestModule::IsValidTestFunction(const char* testFunction)
    {
        return (strncmp(testFunction, TestPrefix, _testPrefixLength) == 0);
    }

    /// Gets the HMODULE for this TestModule if it's been loaded; if it hasn't
    /// yet been loaded, load it.
    /// @return The HMODULE.
    /// @remarks 
    /// OutputDebugString is also hooked for the loaded module here,
    /// once it's been loaded successfully.
    const HMODULE TestModule::GetModule()
    {
        if (_hModule == NULL)
        {
            if (_fileName == NULL)
            {
                _logger.OutputProgramErrorLine(__WFUNCTION__ L": No executable filename provided.");
                return NULL;
            }

            _hModule = LoadLibraryW(_fileName);
            if (_hModule == NULL)
            {
                _logger.OutputWinapiError(GetLastError(), __WFUNCTION__,
                    L"LoadLibraryW failed on ", _fileName);
                return NULL;
            }
            if (_hookOutputDebugString)
            {
                this->HookOutputDebugString();
            }
        }
        return _hModule;
    }

#pragma endregion

#pragma region Support for hooking OutputDebugString

    /// Hook OutputDebugString in the loaded test DLL, so we can redirect 
    /// the output to our logger instead.
    /// @remarks
    /// We must hook both OutputDebugStringA and OutputDebugStringW.  This
    /// function does not return a failure code, because even if we weren't
    /// able to hook it, we're going to proceed with attempting to run the
    /// tests anyway.
    void TestModule::HookOutputDebugString()
    {
        _pStaticLogger = &_logger;

        ImportHooker::HOOKFUNCDESC hookArray[] = 
        {
            { "OutputDebugStringA", (PROC)TestModule::OutputDebugStringAReplacement },
            { "OutputDebugStringW", (PROC)TestModule::OutputDebugStringWReplacement }
        };

        PROC originalFunctionsArray[2] = { NULL };

        char* szImportMod = "KERNEL32.DLL";

        ImportHooker::HookImportedFunctionsByName(
            this->Module, szImportMod, ARRAYSIZE(hookArray), hookArray, 
            originalFunctionsArray, NULL,
            _logger);

        _originalOutputDebugStringA = originalFunctionsArray[0];
        _originalOutputDebugStringW = originalFunctionsArray[1];
    }

    /// Unhook OutputDebugString in the loaded test DLL (i.e. set pointers back
    /// to their original spots in memory) because we are about to end our 
    /// dealings with the library.  Even though we are going to call FreeLibrary,
    /// someone else could still have the library open and we wouldn't want it
    /// to crash when attempting to call these hooked functions which no longer
    /// exist.
    void TestModule::UnhookOutputDebugString()
    {
        ImportHooker::HOOKFUNCDESC hookArray[] = 
        {
            { "OutputDebugStringA", _originalOutputDebugStringA },
            { "OutputDebugStringW", _originalOutputDebugStringW }
        };

        char* szImportMod = "KERNEL32.DLL";

        ImportHooker::HookImportedFunctionsByName(
            this->Module, szImportMod, ARRAYSIZE(hookArray), hookArray, 
            NULL, NULL,
            _logger);

        _pStaticLogger = NULL;
    }

    /// This function is the one that replaces OutputDebugStringA in the 
    /// loaded test DLL, if it was used at all there.
    /// @remarks
    /// This function has the same function signature as OutputDebugStringA.
    /// It uses a static pointer to the Logger instance, set in the non-static
    /// method HookOutputDebugString.
    VOID WINAPI TestModule::OutputDebugStringAReplacement(
        const char* message) ///< NULL-terminated ANSI string that was supposed 
                             ///< to be output by OutputDebugStringA.
    {
        Logger* pLogger = _pStaticLogger;
        if (pLogger == NULL)
        {
            ::fwprintf(stderr, L"[stderr] %S: _pStaticLogger must be set.\n", __FUNCTION__);
            return;
        }

        const size_t bufferLength = 1024;
        wchar_t buffer[bufferLength + 1] = L"";
        size_t messageLength = strlen(message);
        size_t offset = 0;

        while (messageLength - offset > bufferLength)
        {
            // do MultiByteToWideChar on buffer length
            ::MultiByteToWideChar(CP_ACP, 0, message + offset, bufferLength, buffer, bufferLength);
            // write to logger
            pLogger->OutputTrace(buffer);

            offset += bufferLength;
        }
        
        // do MultiByteToWideChar on message length
        ::MultiByteToWideChar(CP_ACP, 0, message + offset, -1, buffer, bufferLength);
        // write to logger
        pLogger->OutputTrace(buffer);
    }

    /// This function is the one that replaces OutputDebugStringW in the 
    /// loaded test DLL, if it was used at all there.
    /// @remarks
    /// This function has the same function signature as OutputDebugStringW.
    /// It uses a static pointer to the Logger instance, set in the non-static
    /// method HookOutputDebugString.
    VOID WINAPI TestModule::OutputDebugStringWReplacement(
        const wchar_t* message)  ///< NULL-terminated wide-char string that was 
                                 ///< supposed to be output by OutputDebugStringW.
    {
        Logger* pLogger = _pStaticLogger;
        if (pLogger == NULL)
        {
            ::fprintf(stderr, "[stderr] %s: _pStaticLogger must be set.\n", __FUNCTION__);
            return;
        }

        pLogger->OutputTrace(message);
    }

    Logger* TestModule::_pStaticLogger = NULL;
#pragma endregion

}