/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file TestResults.h
#pragma once

namespace WinUnitLib
{
    /// This struct stores test scores as they are run.
    struct TestResults
    {
        TestResults();
        void operator+=(const TestResults& rhs);

        unsigned int SucceededCount;
        unsigned int TotalRunCount;
    };
}