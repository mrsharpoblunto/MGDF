/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file FileLogger.cpp

#include "FileLogger.h"

#include <stdarg.h>

namespace WinUnitLib
{
    FileLogger::FileLogger()
        :
    _file(NULL),
    _closeOnExit(false)
    {
    }

    FileLogger::FileLogger(FILE* file, bool closeOnExit /* true */)
    :
    _file(file),
    _closeOnExit(closeOnExit)
    {
        if (file == NULL) 
        { 
            ::fwprintf(stderr, 
                L"[stderr] FileLogger::FileLogger: File is NULL; no output will be written.\n"); 
        }
    }

    FileLogger::~FileLogger(void)
    {
        if (_file != NULL && _closeOnExit)
        {
            ::fclose(_file);
        }
    }

    /// Open a file for output.
    /// @return true if successful; false otherwise.
    bool FileLogger::OpenNewFile(
        const wchar_t* fileName)    ///< Name of file to be opened
    {
        if (_file != NULL  && _closeOnExit)
        {
            ::fclose(_file);
        }
        errno_t result = _wfopen_s(&_file, fileName, L"wtc");
        bool isOpened = (result == 0);

        _closeOnExit = isOpened;

        if (!isOpened)
        {
            ::fwprintf(stderr, L"[stderr] File could not be opened: %s.\n", fileName);
            _file = NULL;
        }

        return isOpened;
    }

    PlainTextLogger& FileLogger::operator<<(const wchar_t* str)
    {
        if (_file)
        {
            ::fwprintf(_file, L"%s", str);
            ::fflush(_file);
        }
        return *this;
    }

    PlainTextLogger& FileLogger::operator<<(const char* str)
    {
        if (_file)
        {
            ::fprintf(_file, "%s", str);
            ::fflush(_file);
        }
        return *this;
    }

    PlainTextLogger& FileLogger::operator<<(unsigned int i)
    {
        if (_file)
        {
            ::fwprintf(_file, L"%u", i);
            ::fflush(_file);
        }
        return *this;
    }
}
