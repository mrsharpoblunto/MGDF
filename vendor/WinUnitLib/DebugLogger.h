/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file DebugLogger.h
#pragma once

#include "PlainTextLogger.h"

namespace WinUnitLib
{
    /// This class is a plain-text logger which writes to OutputDebugString.
    /// @ingroup Loggers
    class DebugLogger : public PlainTextLogger
    {
     protected:
        virtual PlainTextLogger& operator<<(const wchar_t* str);
        virtual PlainTextLogger& operator<<(const char* str);
        virtual PlainTextLogger& operator<<(unsigned int i);
   };
}