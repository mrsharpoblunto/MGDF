/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file TestContainer.h
#pragma once

#include "TestIdentifier.h"

namespace WinUnitLib
{
    /// This abstract class makes it possible to test the TestRunner class without
    /// actually enumerating the exports of an executable.
    class TestContainer
    {
    public:
        /// Get the first test among those being enumerated.
        /// @return false if none.
        virtual bool GetFirst(TestIdentifier&) = 0;

        /// Get the next test among those being enumerated.
        /// @return false if no more.
        virtual bool GetNext(TestIdentifier&) = 0;

        /// Execute the given test and put output (if any) in the provided buffer.
        /// @return true if test succeeds.
        /// @remarks Buffer is ignored by caller if return value is true.
        virtual bool ExecuteTest(
            const TestIdentifier& testIdentifier,
            wchar_t* buffer, 
            size_t cchBuffer) = 0;

    public:
        virtual ~TestContainer() = 0 {}
    };
}