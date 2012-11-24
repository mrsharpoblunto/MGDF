/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */

/**
 *  @file Main.cpp
 *  The implementation file for the entry point and helper functions of WinUnit.exe.
 */

#include <stdarg.h>
#include <windows.h>

#include "TestRunner.h"
#include "FileEnumerator.h"
#include "TestModule.h"

#include "Logger.h"
#include "MasterLogger.h"
#include "FileLogger.h"
#include "ExternalLogger.h"

#include "ErrorHandler.h"
#include "ReturnValues.h"

// The following are helper functions used by wmain.
namespace
{
    // Display usage.  If displayTitle, show the "title" (name & description)
    // of the tool.
    void DisplayHelp(bool displayTitle = true);

    // Display usage, preceded by a specific message regarding an invalid
    // argument.  Return WINUNIT_USAGE_ERROR.
    int DisplayInvalidArgHelp(wchar_t* invalidArgMessage, ...);
    
    // Returns true if "path" is not found or for some other reason we're unable
    // to get file attributes.
    bool IsInvalidFile(const wchar_t* path);

    // Returns true if "path" is a directory.
    bool IsDirectory(const wchar_t* path);

    // Concatenates directory + fileName to produce a full path, and puts it
    // in buffer.  Returns true if successful; false otherwise.
    bool ConcatenateDirectoryAndFile(const wchar_t* directory,
                                     const wchar_t* fileName,
                                     wchar_t* buffer, size_t bufferLength);

    // Gets the "absolute path" if given a path relative to the current 
    // directory, including paths with "." or ".." in them. pFilePart holds
    // a pointer to the filename.
    bool GetAbsolutePath(const wchar_t* path,
                         wchar_t* buffer, DWORD bufferLength,
                         wchar_t** pFilePart);

    // Gets the executable name (without extension) of the executable that
    // started the process (i.e. WinUnit) to be displayed on the usage
    // screen.  The default value is used if there are errors in retrieving
    // the actual value.
    const wchar_t* GetExecutableName(wchar_t* buffer, DWORD bufferLength,
        const wchar_t* defaultValue = L"WinUnit");

    // Gets the product version in string form, from the PRODUCTVERSION section
    // of the version resource.  It's displayed in form X.X.XXXX.X.
    const wchar_t* GetProductVersion(wchar_t* buffer, DWORD bufferLength,
        const wchar_t* defaultValue = L"<unknown>");
}

using namespace WinUnitLib;

int wmain(int argc, wchar_t* argv[])
{
    ErrorHandler::Initialize();

    if (argc == 1) 
    { 
        DisplayHelp();
        return WINUNIT_USAGE_ERROR; 
    }

    bool outputToDebugger = false;
    bool outputToConsole = true;
    const wchar_t* prefix = NULL;
    bool ignoreTestPrefix = false;
    const wchar_t* outputFileName = NULL;
    bool listOnly = false;
    bool nonInteractive = false;
    UINT32 verbosity = 1;
    const wchar_t* customLoggerReferenceString = NULL;
	wchar_t** exactNamesStart = NULL;
	int exactNamesCount = -1;

    int i = 1;
    for ( ; i < argc; i++)
    {
        if (argv[i][0] != '-' && argv[i][0] != '/') { break; }

        switch(argv[i][1])  // '\0' is covered by default case
        {
        case 'q': // do not output to console
            outputToConsole = false;
            break;
        case 'n': // no interactive UI
            nonInteractive = true;
            ErrorHandler::DisableErrorDialogs();
            break;
        case 'b': // output to debugger
            outputToDebugger = true;
            break;
        case 'p': // prefix
            if (++i == argc) 
            { 
                return DisplayInvalidArgHelp(L"-p requires a string to be used as a prefix."); 
            }
            prefix = argv[i];
            break;
		case 'e': // exact names
			if (++i == argc || argv[i][0] != L'(')
			{
				return DisplayInvalidArgHelp(L"-e requires a set of space-delimited test names enclosed in parentheses.");
			}
            if (argv[i][1] != L'\0')
            {
                return DisplayInvalidArgHelp(L"-e requires parentheses to be surrounded by spaces.");
            }
			exactNamesStart = &(argv[++i]);
			for(exactNamesCount = 0; 
				i < argc && argv[i][0] != L')';
				i++)
			{
				exactNamesCount++;
			}
			if (i >= argc || argv[i][0] != L')')
			{
				return DisplayInvalidArgHelp(L"-e argument missing closed parenthesis (parentheses must be surrounded by spaces).");
			}
			break;
        case 'x': // ignore test prefix
            ignoreTestPrefix = true;
            break;
        case 'v': // verbosity
            {
                if (++i == argc) 
                { 
                    return DisplayInvalidArgHelp(L"-v requires a value."); 
                }
                int signedVerbosity = _wtoi(argv[i]);
                if (signedVerbosity < 0) 
                { 
                    return DisplayInvalidArgHelp(L"-v requires a non-negative argument."); 
                }
                verbosity = (unsigned int) signedVerbosity;
            }
            break;
        case 'o': // output filename
            if (++i == argc) { return DisplayInvalidArgHelp(L"-o requires a filename."); }
            if (outputFileName != NULL) { return DisplayInvalidArgHelp(L"-o option can only be used once."); }
            outputFileName = argv[i];
            break;
        case 's': // show test names only; do not run
            listOnly = true;
            break;
        case 'l': // custom logger
            if (++i == argc) { return DisplayInvalidArgHelp(L"-l requires a reference string."); }
            if (customLoggerReferenceString != NULL) { return DisplayInvalidArgHelp(L"-l option can only be used once."); }
            customLoggerReferenceString = argv[i];
            break;
        case '?': // display help
            DisplayHelp(true);
            return WINUNIT_USAGE_ERROR;
        case '-': // environment variable
            {
                wchar_t* variableName = argv[i] + 2;
                if (*variableName == '\0') 
                { 
                    return DisplayInvalidArgHelp(L"-- requires a variable name."); 
                }
                if (++i == argc) 
                { 
                    return DisplayInvalidArgHelp(
                        L"--<variable> requires a value: %s.", variableName); 
                }
                wchar_t* value = argv[i];
                _wputenv_s(variableName, value);
            }
            break;
        default:
            return DisplayInvalidArgHelp(L"Invalid parameter: %s.", argv[i]);
        }
    }
    
    // Check for argument incompatibilities 
    //
    // At least one file/directory argument is required
    if (i == argc)
    {
        return DisplayInvalidArgHelp(L"At least one file or directory argument is required.");
    }
    //
    // Can only use -x option with -s option (disallow running all exports on 
    // a DLL -- listing is okay)
    if (ignoreTestPrefix && !listOnly)
    {
        return DisplayInvalidArgHelp(L"-x option requires -s option.");
    }
	//
	// Cannot use -e option with -x option
	if (ignoreTestPrefix && (exactNamesCount != -1))
	{
		return DisplayInvalidArgHelp(L"Cannot use -e option with -x option.");
	}
	//
	// Cannot use -e option with -p option
    if (prefix && (exactNamesCount != -1))
	{
		return DisplayInvalidArgHelp(L"Cannot use -e option with -p option.");
	}
	// 
	// Cannot use -e option with -s option
	if ((exactNamesCount != -1) && listOnly)
	{
		return DisplayInvalidArgHelp(L"Cannot use -e option with -s option.");
	}
    //
    // Cannot use -p option with -x option
    if (prefix != NULL && ignoreTestPrefix)
    {
        return DisplayInvalidArgHelp(L"Cannot use -p option with -x option.");
    }
    //
    // Verbosity must be at least 1 if listOnly was selected.
    if (listOnly && verbosity < 1)
    {
        verbosity = 1;
    }

    // Set up MasterLogger
    MasterLogger masterLogger(outputToConsole, outputToDebugger, verbosity);
    // Global error handler will write to this logger.
    ErrorHandler::SetLogger(&masterLogger);

    // Add FileLogger to masterLogger if an output file was requested
    FileLogger fileLogger;
    if (outputFileName != NULL)
    {
        bool success = fileLogger.OpenNewFile(outputFileName);
        if (!success)
        {
            return DisplayInvalidArgHelp(L"%s cannot be opened for write.", outputFileName);
        }
        masterLogger.AddLogger(&fileLogger);
    }

    // Add ExternalLogger to masterLogger if one was specified
    ExternalLogger externalLogger;
    if (customLoggerReferenceString != NULL)
    {
        bool success = externalLogger.LoadLogger(customLoggerReferenceString);
        if (!success)
        {
            return DisplayInvalidArgHelp(L"%s was not a valid custom logger string.", customLoggerReferenceString);
        }
        masterLogger.AddLogger(&externalLogger);
    }

    // Begin running tests...
    TestRunner testRunner(masterLogger);

    testRunner.ListOnly = listOnly;
    testRunner.IgnoreTestPrefix = ignoreTestPrefix;

    TestResults cumulativeResults;

    // We are assuming that the rest of the arguments are files or directories.
    for (; i < argc; i++)
    {
        wchar_t* arg = argv[i];

        wchar_t absolutePath[MAX_PATH] = L"";
        wchar_t* filePart = NULL;

        // In case something like ..\debug or ..\debug\MyTests.dll was typed in
        // here is where it gets converted to an absolute path.
        if (!GetAbsolutePath(arg, absolutePath, MAX_PATH, &filePart))
        {
            masterLogger.OutputProgramErrorLine(__WFUNCTION__ L": Unable to determine absolute path: ", arg);
            continue;
        }
        
        if (IsInvalidFile(absolutePath))
        {
            masterLogger.OutputProgramErrorLine(__WFUNCTION__ L": Not a valid file or directory: ", absolutePath);
            continue;
        }

        // If it is a directory, we want to enumerate through all the DLLs in it.
        if (IsDirectory(arg))
        {
            wchar_t* directory = absolutePath;

            FileEnumerator fileEnumerator(masterLogger, directory);
            for(const wchar_t* fileName = fileEnumerator.GetFirst();
                fileName != NULL;
                fileName = fileEnumerator.GetNext())
            {
                wchar_t fullPathName[MAX_PATH] = L"";

                // FileEnumerator just returns the file names without their 
                // qualifying path.
                if (!ConcatenateDirectoryAndFile(directory, fileName, fullPathName, MAX_PATH))
                {
                    masterLogger.OutputProgramErrorLine(
                        __WFUNCTION__ L": Full path name could not be composed: ", 
                        directory, L"+", fileName);
                    continue;
                }

                cumulativeResults += testRunner.ProcessTestFile(
					fullPathName, fileName, prefix, exactNamesStart, exactNamesCount);

            }
        }
        else
        {
            // Else it's an indivdual DLL
            wchar_t* fileName = filePart;
            wchar_t* fullPathName = absolutePath;

            cumulativeResults += testRunner.ProcessTestFile(
                fullPathName, fileName, prefix, exactNamesStart, exactNamesCount);
        }
    }

    if (!listOnly)
    {
        masterLogger.OutputFinalResults(cumulativeResults.SucceededCount, cumulativeResults.TotalRunCount);
    }

    ErrorHandler::SetLogger(NULL);

    return 
        ((cumulativeResults.TotalRunCount - cumulativeResults.SucceededCount == 0) ?
         WINUNIT_EXIT_SUCCESS :
         WINUNIT_EXIT_TEST_FAILURE);
}

// Implementation of helper functions
namespace
{
    /// @return true if the given path is invalid; false otherwise.
    bool IsInvalidFile(const wchar_t* path)
    {
        return (::GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES);
    }

    /// @return true if the given path is a directory; false otherwise.
    bool IsDirectory(const wchar_t* path)
    {
        DWORD fileAttributes = ::GetFileAttributesW(path);
        return ((fileAttributes != INVALID_FILE_ATTRIBUTES) &&
                (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
    }

    /// Concatenate a directory and a filename to produce a full path.
    /// @return true if successful; false otherwise.
    bool ConcatenateDirectoryAndFile(
        const wchar_t* directory,   ///< Directory name, optionally followed by backslash.
        const wchar_t* fileName,    ///< Filename to be concatenated.
        wchar_t* buffer,            ///< Buffer to receive concatenated director + filename.
        size_t bufferLength)        ///< Size of buffer in wide-chars.
    {
        // Concatenate directory & fileName
        ::wcscpy_s(buffer, bufferLength, directory);
        if (directory[::wcslen(directory) - 1] != L'\\')
        {
            ::wcscat_s(buffer, bufferLength, L"\\");
        }

        errno_t result = ::wcscat_s(buffer, bufferLength, fileName);

        return (result == 0);
    }

    /// Convert a relative path (or one containing "." or "..") into an
    /// absolute path.
    /// @return true if successful; false otherwise.
    bool GetAbsolutePath(const wchar_t* path, ///< [in] The relative path.
                         wchar_t* buffer,     ///< The buffer to receive the absolute path.
                         DWORD bufferLength,  ///< The size of the buffer in wide-chars.
                         wchar_t** pFilePart) ///< A pointer to the start of the filename
                                              ///< part of the output.
    {
        // Use current directory
        DWORD size = 
            ::GetFullPathNameW(path, 
                    bufferLength, buffer, pFilePart);

        return (size <= bufferLength);
    }

    /// Display usage.
    void DisplayHelp(
        bool displayTitle /* true */) ///< Whether or not to display the title
                                      ///< and description of this tool before
                                      ///< displaying the usage
    {
        wchar_t buffer[_MAX_FNAME] = L"";
        const wchar_t* executableName = GetExecutableName(buffer, 
            ARRAYSIZE(buffer));

        if (displayTitle)
        {
            wchar_t versionBuffer[128] = L"";
            wprintf(L"\n"
                    L"  %s - Run native C++ unit tests\n"
                    L"\n"
                    L"  Version: %s\n"
                    L"  Author: Maria Blees (maria.blees@microsoft.com)\n",
                    executableName, 
                    GetProductVersion(versionBuffer,
                                      ARRAYSIZE(versionBuffer)));
           
        }
        wprintf(L"\n"
                L"Usage:\n"
                L"  %s -s [-l <ref>] [-x | -p <prefix>] [-o <file>] {<dir> | <dll>}\n"
				L"  %s [-p <prefix> | -e ( <testName>* )] [-v <verbosity>] [-n] [-q]\n"
				L"        [-l <ref>] [-o <file>] [--<var> <value>]* {<dir> | <dll>}*\n"
                L"\n"
                L"Options:\n"
                L"  -s : show test names only; do not run\n"
                L"  -l <ref> : custom logger\n"
                L"  -q : do not send output to console\n"
                L"  -n : no interactive UI\n"
                L"  -b : send output to debugger (OutputDebugString)\n"
                L"  -x : show all exports in raw form\n"
                L"  -o <file> : output file name (default is stdout)\n"
                L"  -p <prefix> : only run tests whose names start with prefix (case-insensitive)\n"
				L"  -e ( <testName>* ) : only run tests whose exact names are specified\n"
				L"     space-delimited between parens (case-sensitive)\n"
                L"  -v <number> : verbosity (default is 0)\n"
                L"  --<var> <value> : sets environment variable var to value (in current process)\n"
                L"  <dll> : individual name of DLL to execute on\n"
                L"  <dir> : name of directory to run DLLs in\n"
                L"\n"
                L"Note on custom logger:\n"
                L"  \"ref\" is in the form of the path to a DLL that implements\n"
                L"  one or more of the logger functions, followed optionally by\n"
                L"  a colon and an initialization string to be passed to the\n"
                L"  logger's Initialize function.\n"
                L"\n"
                L"Exit codes:\n"
                L"  %d: No errors (any tests run succeeded)\n"
                L"  %d: One or more tests failed\n"
                L"  %d: An unhandled exception caused premature termination\n"
                L"  %d: Usage errror\n",
                executableName, executableName,
                WINUNIT_EXIT_SUCCESS,
                WINUNIT_EXIT_TEST_FAILURE,
                WINUNIT_EXIT_UNHANDLED_EXCEPTION,
                WINUNIT_USAGE_ERROR
                );
    }

    /// Display usage, preceded by a message regarding the specific error in 
    /// usage that was observed.
    /// @return WINUNIT_USAGE_ERROR, for easy exit from wmain.
    int DisplayInvalidArgHelp(
        wchar_t* invalidArgMessage, ///< Printf-style formatted message string
                                    ///< to be displayed before usage message.
        ...)                        ///< Optional args to format string.
    {
        wprintf(L"\n  ");

        va_list argList;
        va_start(argList, invalidArgMessage);
        ::vwprintf_s(invalidArgMessage, argList);
        va_end(argList);

        wprintf(L"\n");
        DisplayHelp(false);
        return WINUNIT_USAGE_ERROR;
    }

    
    /// Gets the executable name (without extension) of the executable that
    /// started the process (i.e. WinUnit) to be displayed on the usage
    /// screen.
    /// @return Name of executable, without extension, or defaultValue if 
    /// actual value cannot be retrieved.
    const wchar_t* GetExecutableName(
        wchar_t* buffer,            ///< Buffer to receive executable name.
        DWORD bufferLength,         ///< Size of buffer, in wide characters.
        const wchar_t* defaultValue ///< Value to be returned in case of error
                                    ///< (default: L"WinUnit")
    )
    {
        wchar_t executablePath[MAX_PATH] = L"";
        DWORD charCount = GetModuleFileNameW(NULL, executablePath, 
            ARRAYSIZE(executablePath));
        if (charCount == 0 || charCount >= ARRAYSIZE(executablePath))
        {
            return defaultValue;
        }

        // _MAX_FNAME
        errno_t result = _wsplitpath_s(executablePath, 
            NULL, 0,              // drive
            NULL, 0,              // directory
            buffer, bufferLength, // filename
            NULL, 0);             // extension

        if (result != 0)
        {
            return defaultValue;
        }

        return buffer;
    }

    /// Gets the product version in string form, from the PRODUCTVERSION section
    /// of the version resource.  
    /// @return Product version in form X.X.XXXX.X, or defaultValue if error.
    const wchar_t* GetProductVersion(
        wchar_t* buffer,             ///< Buffer to receive product version string.
        DWORD bufferSize,            ///< Size of buffer in wide characters.
        const wchar_t* defaultValue  ///< Value to be returned in case of error
        )
    {
        wchar_t executablePath[MAX_PATH] = L"";
        DWORD charCount = GetModuleFileNameW(NULL, executablePath, 
            ARRAYSIZE(executablePath));
        if (charCount == 0 || charCount >= ARRAYSIZE(executablePath))
        {
            return defaultValue;
        }

        DWORD unused = 0;
        DWORD size = GetFileVersionInfoSizeW(executablePath, &unused);
        if (size == 0) { return defaultValue; }

        const UINT32 versionInfoMaxSize = 2048;
        char versionInfoBlock[versionInfoMaxSize] = { '\0' };
        if (size > versionInfoMaxSize) { return defaultValue; }
        
        if (!GetFileVersionInfoW(executablePath, 
            0 /* unused */, 
            versionInfoMaxSize, (void*)versionInfoBlock))
        {
            return defaultValue;
        }

        VS_FIXEDFILEINFO* pFileInfo = NULL;
        UINT length = 0;
        if (!VerQueryValueW((void*)versionInfoBlock, L"\\", 
            (void**)&pFileInfo, &length))
        {
            return defaultValue;
        }
        
        if (-1 == swprintf_s(buffer, bufferSize, L"%01hu.%01hu.%04hu.%01hu", 
            HIWORD(pFileInfo->dwProductVersionMS),
            LOWORD(pFileInfo->dwProductVersionMS),
            HIWORD(pFileInfo->dwProductVersionLS),
            LOWORD(pFileInfo->dwProductVersionLS)))
        {
            return defaultValue;
        }

        return buffer;
    }
}
