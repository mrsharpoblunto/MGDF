/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file TestIdentifier.cpp

#include "TestIdentifier.h"

using namespace WinUnitLib;

TestIdentifier::TestIdentifier(
    HMODULE hModule,                ///< HMODULE of the loaded executable that contains the test.
    const char* originalExportName, ///< The original export name (including "TEST_" prefix).
    const char* testName)           ///< The display name of the test (export name minus "TEST_" prefix).
    : 
_hModule(hModule),
_originalExportName(originalExportName), 
_testName(testName)
{}

TestIdentifier::TestIdentifier()
:
_hModule(0),
_originalExportName(0), 
_testName(0)
{}

TestIdentifier& TestIdentifier::operator=(const TestIdentifier& rhs)
{
    this->_hModule = rhs._hModule;
    this->_originalExportName = rhs._originalExportName;
    this->_testName = rhs._testName;

    return *this;
}
