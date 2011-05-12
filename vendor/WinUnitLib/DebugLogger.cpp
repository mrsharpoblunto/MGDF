/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file DebugLogger.cpp

#include "DebugLogger.h"

#include <stdio.h>
#include <windows.h>

namespace WinUnitLib
{
    PlainTextLogger& DebugLogger::operator<<(const wchar_t* str)
    {
        OutputDebugStringW(str);
        return *this;
    }

    PlainTextLogger& DebugLogger::operator<<(const char* str)
    {
        OutputDebugStringA(str);
        return *this;
    }

    PlainTextLogger& DebugLogger::operator<<(unsigned int i)
    {
        wchar_t buffer[11] = L"";
        ::swprintf_s(buffer, L"%u", i);
        OutputDebugStringW(buffer);
        return *this;
    }
}