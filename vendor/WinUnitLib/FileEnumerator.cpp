/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file FileEnumerator.cpp

#include "FileEnumerator.h"
#include "Logger.h"

using namespace WinUnitLib;

FileEnumerator::FileEnumerator(
   Logger& logger,                          ///< Where output goes
   const wchar_t* directory,                ///< Directory to look in
   const wchar_t* fileMatcher /* *.dll */)  ///< Wildcard to match files against
:
_directory(directory),
_fileMatcher(fileMatcher),
_logger(logger),
_hFind(INVALID_HANDLE_VALUE)
{
    // Zero out _findData
    ::SecureZeroMemory((void*)&_findData, sizeof(_findData));

    // Copy directory + wildcard to directoryMask
    ::wcscpy_s(this->_directoryMask, directory);
    // Ensure directory name ends with "\".
    if (directory[::wcslen(directory) - 1] != L'\\')
    {
        ::wcscat_s(this->_directoryMask, L"\\");
    }
    ::wcscat_s(this->_directoryMask, fileMatcher);
}

FileEnumerator::~FileEnumerator(void)
{
    CloseFindHandleIfOpen();
}

/// Get first file in directory that matches the wildcard.
/// @return Name of file (without directory), or NULL if none.
/// @remarks Return value is not copied but exists in member _findData.  It is
/// only valid until the next time GetFirst or GetNext is called.
const wchar_t* FileEnumerator::GetFirst()
{
    // We allow GetFirst to be called more than once. 
    CloseFindHandleIfOpen();

    // Look for the first file.
    _hFind = ::FindFirstFileW(_directoryMask, &_findData);

    if (_hFind == INVALID_HANDLE_VALUE)
    {
        DWORD error = ::GetLastError();
        // File not found is not considered an error
        if (error != ERROR_FILE_NOT_FOUND)
        {
            _logger.OutputWinapiError(error, __WFUNCTION__,
                L"FindFirstFile failed with ",
                this->_directoryMask);
        }
        return NULL;
    }

    const wchar_t* fileName = _findData.cFileName;
    if (IsDirectory(_findData)) { fileName = GetNext(); }

    return fileName;
}

/// Gets next filename in directory.
/// @return Filename (with no directory), or NULL if no more.
/// @remarks The result is only valid until GetFirst or GetNext are called again.
/// GetFirst must be called once before GetNext will succeed.
const wchar_t* FileEnumerator::GetNext()
{
    if (_hFind == INVALID_HANDLE_VALUE)
    {
        _logger.OutputProgramErrorLine(
            __WFUNCTION__ L": GetFirst() failed or has not been called, or all files have been enumerated.");
        return NULL;
    }
    BOOL success = FALSE;
    const wchar_t* fileName = NULL;
    while((success = ::FindNextFileW(_hFind, &_findData)) != 0)
    {
        if (IsDirectory(_findData)) { continue; }

        fileName = _findData.cFileName;
        break;
    } 

    if (!success) 
    {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_NO_MORE_FILES)
        {
            _logger.OutputWinapiError(dwError, __WFUNCTION__,
                L"FindNextFileW failed");
        }
    }

    return fileName;
}

bool FileEnumerator::IsDirectory(const WIN32_FIND_DATAW& findData)
{
    return ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

void FileEnumerator::CloseFindHandleIfOpen()
{
    if (_hFind != INVALID_HANDLE_VALUE)
    {
        BOOL closed = ::FindClose(_hFind);
        if (!closed)
        {
            _logger.OutputWinapiError(GetLastError(), __WFUNCTION__,
                L"FindClose failed on file handle for directory ",
                this->_directory);
        }
        _hFind = INVALID_HANDLE_VALUE;
    }
}
