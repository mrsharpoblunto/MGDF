/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file TestIdentifier.h
#pragma once

#include <windows.h>

namespace WinUnitLib
{
    /// This class holds enough information about a test to be able to run it.
    /// @remarks The _originalExportName and the _testName are ANSI strings because 
    /// that is how they come out of the exports table.  Note that these are
    /// actual pointers into the exports table, so they should be used 
    /// or copied immediately.  TestName is the original export name minus
    /// the prefix we recognize tests by.
    class TestIdentifier
    {
    private:
        HMODULE _hModule;             ///< HMODULE of the loaded executable that contains the test.
        const char* _originalExportName;    ///< The original export name (including "TEST_" prefix).
        const char* _testName;              ///< The display name of the test (export name minus "TEST_" prefix).

    public:
        TestIdentifier();
        TestIdentifier(
            HMODULE hModule,
            const char* originalExportName,
            const char* testName);
        
        /// Module gets the HMODULE of the loaded executable the test is found in, 
        /// so it can be run.
        __declspec( property( get=GetModule ) ) 
        const HMODULE Module;
        const HMODULE GetModule() const;

        /// OriginalExportName gets the original name of the exported 
        /// test function.
        __declspec( property( get=GetOriginalExportName ) ) 
        const char* OriginalExportName;
        const char* GetOriginalExportName() const;

        /// TestName gets the name of the test (an ANSI string which is
        /// the original export name minus the TEST_ prefix).
        __declspec( property( get=GetTestName ) ) 
        const char* TestName;
        const char* GetTestName() const;

        TestIdentifier& operator=(const TestIdentifier&);
    };

    inline const HMODULE TestIdentifier::GetModule() const
    {
        return _hModule;
    }

    inline const char* TestIdentifier::GetOriginalExportName() const
    {
        return _originalExportName;
    }

    inline const char* TestIdentifier::GetTestName() const
    {
        return _testName;
    }
}