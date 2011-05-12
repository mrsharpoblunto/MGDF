/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file FileLogger.h
#pragma once

#include "PlainTextLogger.h"

#include <stdio.h>

namespace WinUnitLib
{
    /// This class is a PlainTextLogger which writes to a file stream.
    /// @ingroup Loggers
    class FileLogger : public PlainTextLogger
    {
    private:
        FILE* _file;            ///< The file to be written to.
        bool _closeOnExit;      ///< Whether the file should be closed on exit.

    public:
        FileLogger();
        FileLogger(FILE* file, bool closeOnExit = true);
        bool OpenNewFile(const wchar_t* fileName);
        virtual ~FileLogger(void);

    protected:
        virtual PlainTextLogger& operator<<(const wchar_t* str);
        virtual PlainTextLogger& operator<<(const char* str);
        virtual PlainTextLogger& operator<<(unsigned int i);
   };
}