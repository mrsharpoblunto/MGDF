/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file FileEnumerator.h
#pragma once

#include <windows.h>
#include <stdio.h>

namespace WinUnitLib
{
    class Logger;

    /// This class enumerates all files in a directory that match the given
    /// DOS-style wildcard "fileMatcher".
    class FileEnumerator
    {
    protected:
        const wchar_t* _directory;       ///< The directory to enumerate the files in
        const wchar_t* _fileMatcher;     ///< Only return files that match this string (can be DOS wildcard, e.g. *.dll)
        Logger& _logger;                 ///< Place to output error messages
        HANDLE _hFind;                   ///< A type of handle used by the Find* functions

        wchar_t _directoryMask[MAX_PATH]; ///< Holds the path+wildcard to match files against
        WIN32_FIND_DATAW _findData;       ///< Holds the "find data" of the current file
    public:
        FileEnumerator(
            Logger& logger,             
            const wchar_t* directory,   
            const wchar_t* fileMatcher = L"*.dll"); 
        ~FileEnumerator(void);
    public:
        const wchar_t* GetFirst();
        const wchar_t* GetNext();
    private:
        static bool IsDirectory(const WIN32_FIND_DATAW& findData);
        void CloseFindHandleIfOpen();
    private:
        FileEnumerator& operator=(const FileEnumerator&);
    };

}