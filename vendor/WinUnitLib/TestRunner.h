/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file TestRunner.h
#pragma once

#include <windows.h>
#include <stdio.h>
#include "TestResults.h"

namespace WinUnitLib
{
    class TestContainer;
    class TestIdentifier;
    class Logger;

    /// This is the class responsible for running a set of tests.
    class TestRunner
    {
    private:
        Logger& _logger;      ///< Logger to log test output to.

        bool _ignoreTestPrefix; ///< True if "TEST_" doesn't matter (for listing).
        bool _listOnly;         ///< True if tests are not supposed to be run, only listed.

    public:
        TestRunner(Logger& logger);

        // Process (run or display) all tests provided by a TestContainer that 
        // start with startsWith or matches one of the exactNames.
        TestResults ProcessTests(TestContainer&, const wchar_t* startsWith = NULL,
            wchar_t** exactNames = NULL, int exactNameCount = -1);
            
        /// Compare two strings (up to length N) of different character types.
        template<class T, class U>
        static int StringCompareCaseInsensitiveN(const T* string1, const U* string2, size_t n);

        /// Compare two strings of different character types.
        template<class T, class U>
        static int StringCompareExact(const T* string1, const U* string2);

        TestResults ProcessTestFile(
            const wchar_t* fullPathName,
            const wchar_t* fileName,
            const wchar_t* onlyRunTestsStartingWith,
			wchar_t** exactNames,
			int exactNameCount);

        __declspec(property(put=SetListOnly)) bool ListOnly;
        void SetListOnly(bool listOnly);
        __declspec(property(put=SetIgnoreTestPrefix)) bool IgnoreTestPrefix;
        void SetIgnoreTestPrefix(bool ignoreTestPrefix);

    private:
        // Return true if the given test name matches either the prefix (case-insensitive)
        // or the exact match names (case-sensitive); otherwise returns false;
        bool TestRunner::TestNameMatchesRequirements(
            const char* testName,                  // Name of test to check for match.
            const wchar_t* startsWith,             // Prefix to check for (if not NULL).
            wchar_t** exactMatches,                // List of exact matches (or NULL if none).
            int exactMatchCount);                  // Number of exact matches.

    private:
        // Not implemented
        TestRunner(const TestRunner&);
        TestRunner& operator=(const TestRunner&);
    };

    template<class T, class U>
    inline int TestRunner::StringCompareCaseInsensitiveN(const T* string1, const U* string2, size_t n)
    {
        size_t i = 0;
        while((i < n - 1) && string1[i] && string2[i] && 
            ::tolower((int)string1[i]) == ::tolower((int)string2[i]))
        {
            i++;
        }
        return (::tolower((int)string1[i]) - ::tolower((int)string2[i]));
    }

    template<>
    inline int TestRunner::StringCompareCaseInsensitiveN(const char* string1, const char* string2, size_t n)
    {
        return ::_strnicmp(string1, string2, n);
    }

    template<class T, class U>
    inline int TestRunner::StringCompareExact(const T* string1, const U* string2)
    {
        size_t i = 0;
        while(string1[i] && string2[i] && 
            ((int)string1[i]) == ((int)string2[i]))
        {
            i++;
        }
        return ((int)string1[i] - (int)string2[i]);
    }

    template<>
    inline int TestRunner::StringCompareExact(const char* string1, const char* string2)
    {
        return ::strcmp(string1, string2);
    }

    inline void TestRunner::SetListOnly(bool listOnly)
    {
        _listOnly = listOnly;
    }

    inline void TestRunner::SetIgnoreTestPrefix(bool ignoreTestPrefix)
    {
        _ignoreTestPrefix = ignoreTestPrefix;
    }
}