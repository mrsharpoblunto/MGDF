/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file TestRunner.cpp

#include "TestRunner.h"
#include "TestIdentifier.h"
#include "TestContainer.h"
#include "Logger.h"
#include "TestConstants.h"
#include "StructuredExceptions.h"

#include "TestModule.h"

using namespace WinUnitLib;
using namespace TestConstants;

TestRunner::TestRunner(
  Logger& logger) ///< Logger for logging test output to
:
_logger(logger),
_ignoreTestPrefix(false),
_listOnly(false)
{
}

/// This function takes a TestContainer and processes all the tests provided that
/// start with startsWith.  If startsWith is NULL (the default), all tests are
/// processed.  If _listOnly is true, tests are not run but are just output.  
/// @return A TestResults object indicating the number of tests run and passed.
TestResults TestRunner::ProcessTests(
    TestContainer& testEnumerator,         ///< Provides the tests to be processed.
    const wchar_t* startsWith /* NULL */,  ///< Prefix to check for (if not NULL).
    wchar_t** exactMatches /* NULL */,     ///< List of exact matches (or NULL if none).
    int exactMatchCount /* -1 */)          ///< Number of exact matches.
{
    TestResults testResults;

    TestIdentifier testIdentifier;
    for(bool succeeded = testEnumerator.GetFirst(testIdentifier);
        succeeded;
        succeeded = testEnumerator.GetNext(testIdentifier))
    {
        // Skip if test name was not among those specified by prefix or exact match, if any
        if (!TestNameMatchesRequirements(testIdentifier.TestName, startsWith, exactMatches, exactMatchCount))
        {
            continue;
        }

        // The pre-test output is used for listing tests as well.
        _logger.OutputTestNamePreTest(testIdentifier.TestName);

        // If list only was specified, we're done; we're not running the test.
        if (_listOnly) { continue; }

        // Now we'll actually run the test.
        bool success = false;
        EXCEPTION_RECORD exceptionRecord = { 0 };
        __try
        {
            wchar_t buffer[MaxAssertMessageLength] = L"";
            success = testEnumerator.ExecuteTest(testIdentifier, buffer, ARRAYSIZE(buffer));

			if (!success) { _logger.OutputTestError(testIdentifier.TestName,buffer); }
        }
        __except(StructuredExceptions::Filter(GetExceptionInformation(), &exceptionRecord))
        {
            // The "extra information" is only interesting for access violations
            // --it indicates read vs. write--otherwise just set to 0.
            DWORD extraInformation = (
                 (exceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) ?
                 (DWORD)exceptionRecord.ExceptionInformation[0] :
                 0);
            
				 _logger.OutputTestError(testIdentifier.TestName,StructuredExceptions::GetDescription(
                 exceptionRecord.ExceptionCode, extraInformation));
        }

        if (success)
        {
            testResults.SucceededCount++;
        }
        testResults.TotalRunCount++;
        _logger.OutputTestNamePostTest(testIdentifier.TestName, success);
    }

    return testResults;
}

TestResults TestRunner::ProcessTestFile(
    const wchar_t* fullPathName,
    const wchar_t* fileName,
    const wchar_t* onlyRunTestsStartingWith,
    wchar_t** exactNames,
    int exactNameCount)
{
    _logger.OutputTestExecutableNamePreTest(fullPathName, fileName);

    TestModule testModule(_logger, fullPathName, _ignoreTestPrefix);
    TestResults results = ProcessTests(testModule, onlyRunTestsStartingWith, exactNames, exactNameCount);

    if (!_listOnly)
    {
        _logger.OutputTestExecutableNamePostTest(fullPathName, fileName, 
            results.SucceededCount, results.TotalRunCount);
    }

    return results;
}

bool TestRunner::TestNameMatchesRequirements(
    const char* testName,                  
    const wchar_t* startsWith,             
    wchar_t** exactMatches,                
    int exactMatchCount)                   
{
    if (!startsWith && !exactMatches) { return true; }

    size_t startsWithLength = (startsWith != NULL ? ::wcslen(startsWith) : 0);

    // If it matches the prefix, return true.
    if (startsWith && 
        StringCompareCaseInsensitiveN(testName, startsWith, startsWithLength) == 0)
    {
        return true;
    }

    // If it matches one of the exact matches, return true.
    if (exactMatches)
    {
        for (int i = 0; i < exactMatchCount; i++)
        {
            if (StringCompareExact(testName, exactMatches[i]) == 0)
            {
                return true;
            }
        }
    }

    // Otherwise, return false.
    return false;
}

