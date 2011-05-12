/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file TestResults.cpp

#include "TestResults.h"

using namespace WinUnitLib;

TestResults::TestResults()
:
SucceededCount(0),
TotalRunCount(0)
{}

void TestResults::operator+=(const TestResults& rhs)
{
    SucceededCount += rhs.SucceededCount;
    TotalRunCount += rhs.TotalRunCount;
}
