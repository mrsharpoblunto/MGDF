/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file ExternalLogger.cpp

#include "ExternalLogger.h"
#include "StructuredExceptions.h"

#include <stdio.h>

// This defines all the typedefs needed for the functions implemented by custom
// loggers.
#define WINUNIT_LOGGER_FUNCTION(returntype, func) typedef returntype (__cdecl * func##Ptr)
#include "WinUnitLogger.h"

using namespace WinUnitLib;

// Calling custom logger functions includes catching structured exceptions so
// the logger does not crash the program.
#define CALL_FUNCTION_IF_EXISTS(hmodule, func, ...)                      \
{                                                                       \
    func##Ptr pfn = (func##Ptr)::GetProcAddress(hmodule, #func);        \
    EXCEPTION_RECORD exceptionRecord = { 0 };                           \
    __try                                                               \
    {                                                                   \
        if (pfn) { pfn(__VA_ARGS__); }                                  \
    }                                                                   \
    __except(StructuredExceptions::Filter(GetExceptionInformation(), &exceptionRecord)) \
    {                                                                   \
        DWORD extraInformation = (                                      \
          (exceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) ?  \
          (DWORD)exceptionRecord.ExceptionInformation[0] :              \
          0);                                                           \
        ::fprintf(stderr,                                               \
            "[stderr] Custom logger has thrown an exception"            \
            " and is being disabled (%s). %S\n", __FUNCTION__,          \
            StructuredExceptions::GetDescription(                       \
                exceptionRecord.ExceptionCode, extraInformation));      \
        hmodule = NULL;                                                 \
    }                                                                   \
}

ExternalLogger::ExternalLogger()
:
_hmodule(NULL)
{
}

ExternalLogger::~ExternalLogger()
{
    if (_hmodule != NULL)
    {
        ::FreeLibrary(_hmodule);
    }
}

/// Load logger given a reference string (passed on the command line via 
/// WinUnit -l).
bool ExternalLogger::LoadLogger(
   const wchar_t* reference) ///< String in the form of {loggerDll}[:{initializationString}]
{
    const wchar_t* delimiterPosition = wcschr(reference, L':');
    if (delimiterPosition != NULL)
    {
        wchar_t file[MAX_PATH] = L"";
        ::wcsncpy_s(file, ARRAYSIZE(file), reference, (delimiterPosition - reference));
        return LoadLogger(file, delimiterPosition + 1);
    }
    else
    {
        return LoadLogger(reference, NULL);
    }
}

/// Load custom logger given the file name of a DLL implementing it, and an 
/// initialization string.
bool ExternalLogger::LoadLogger(
    const wchar_t* file,                  ///< Path to DLL that implements custom logger.
    const wchar_t* initializationString)  ///< String that is to be passed to custom logger's Initialize function.
{
    // You can only load one custom logger DLL in a single instance of
    // ExternalLogger.
    if (_hmodule != NULL) { return false; }

    _hmodule = LoadLibraryW(file);
    if (_hmodule == NULL)
    {
        return false;
    }

    CALL_FUNCTION_IF_EXISTS(_hmodule, Initialize, initializationString);

    return true;
}

void ExternalLogger::OutputTestExecutableNamePreTest(
    const wchar_t* fullPath, 
    const wchar_t* nameOfExecutable)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputTestExecutableNamePreTest, fullPath, nameOfExecutable);
}

void ExternalLogger::OutputTestExecutableNamePostTest(
    const wchar_t* fullPath, 
    const wchar_t* nameOfExecutable, 
    unsigned int succeeded, 
    unsigned int totalRun)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputTestExecutableNamePostTest, fullPath, nameOfExecutable, succeeded, totalRun);
}

void ExternalLogger::OutputTestNamePreTest(const char* testName)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputTestNamePreTest, testName);
}

void ExternalLogger::OutputTestNamePostTest(
    const char* testName, 
    bool passed)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputTestNamePostTest, testName, passed);
}

void ExternalLogger::OutputTrace(const wchar_t* message)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputTrace, message);
}

void ExternalLogger::OutputTestError(const char* testName,const wchar_t* message)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputTestError, testName, message);
}

void ExternalLogger::OutputProgramErrorLine(
    const wchar_t* msg1,
    const wchar_t* msg2,
    const wchar_t* msg3,
    const wchar_t* msg4)
{
    if (_hmodule == NULL) { return; }

    wchar_t buffer[1024] = L"";
    _snwprintf_s(buffer, ARRAYSIZE(buffer), _TRUNCATE, L"%s%s%s%s",
        msg1,
        (msg2 != NULL) ? msg2 : L"",
        (msg3 != NULL) ? msg3 : L"",
        (msg4 != NULL) ? msg4 : L"");

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputProgramErrorLine, buffer);
}

void ExternalLogger::OutputFinalResults(
    unsigned int succeeded, 
    unsigned int totalRun)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, OutputFinalResults, succeeded, totalRun);
}

void ExternalLogger::SetVerbosity(
    unsigned int verbosity)
{
    if (_hmodule == NULL) { return; }

    CALL_FUNCTION_IF_EXISTS(_hmodule, SetVerbosity, verbosity);
}
