/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file TestConstants.h
/// 
///  This file contains constants/typedefs that are specifically related to
///  the implementation of test DLLs.  It's here in one place to make it easy
///  to check for mismatches.

#pragma once

namespace TestConstants
{
    /// This (MaxAssertMessageLength) happens to be the maximum size an assert 
    /// message can be.  If that constant in WinUnit.h changed, however, 
    /// it wouldn't cause disastrous consequences here.
    enum { MaxAssertMessageLength = 1024 };

    /// The prefix that export names must start with in order to be considered
    /// tests.
    __declspec(selectany) const char* TestPrefix = "TEST_";

    /// The function prototype for test exports.
    typedef bool (__cdecl *TestPtr)(wchar_t* buffer, size_t cchBuffer);
}