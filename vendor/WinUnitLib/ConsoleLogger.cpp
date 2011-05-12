/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file ConsoleLogger.cpp

#include "ConsoleLogger.h"

using namespace WinUnitLib;

ConsoleLogger::ConsoleLogger()
:
_stdoutLogger(stdout, false),
_stderrLogger(stderr, false)
{
}

/// Outputs the full path and just the name of the executable tests
/// are being run from, before running the tests.
void ConsoleLogger::OutputTestExecutableNamePreTest(
    const wchar_t* fullPath, 
    const wchar_t* nameOfExecutable)
{
    _stdoutLogger.OutputTestExecutableNamePreTest(fullPath, nameOfExecutable);
}

/// Outputs the full path and just the name of the executable tests
/// are being run from, after running the tests.
void ConsoleLogger::OutputTestExecutableNamePostTest(
    const wchar_t* fullPath, 
    const wchar_t* nameOfExecutable, 
    unsigned int succeeded,       ///< Number of tests succeeded
    unsigned int totalRun)        ///< Number of tests run
{
    if (succeeded != totalRun)
    {
        _stderrLogger.OutputTestExecutableNamePostTest(fullPath, 
            nameOfExecutable, succeeded, totalRun);
    }
    else
    {
        _stdoutLogger.OutputTestExecutableNamePostTest(
            fullPath, nameOfExecutable,
            succeeded, totalRun);
    }
}

/// Outputs the name of the test about to be run.
void ConsoleLogger::OutputTestNamePreTest(const char* testName)
{
    _stdoutLogger.OutputTestNamePreTest(testName);
}

/// Outputs the name of the test after it has been run, and whether 
/// or not it passed.
void ConsoleLogger::OutputTestNamePostTest(
    const char* testName, 
    bool passed)
{
    if (!passed)
    {
        _stderrLogger.OutputTestNamePostTest(testName, passed);
    }
    else
    {
        _stdoutLogger.OutputTestNamePostTest(testName, passed);
    }
}

/// Output a trace message.  This comes from any OutputDebugString
/// calls that have been made in the test executable.
void ConsoleLogger::OutputTrace(const wchar_t* message)
{
    _stdoutLogger.OutputTrace(message);
}

/// Output a message from a failed test.
void ConsoleLogger::OutputTestError(const char* testName,const wchar_t* message)
{
    _stderrLogger.OutputTestError(testName,message);
}

/// Output a series of up to four strings (followed by a newline) that
/// describe an unexpected occurrence in the program.  This could
/// be implemented with an assert, if interactivity is desired.
void ConsoleLogger::OutputProgramErrorLine(
    const wchar_t* msg1,
    const wchar_t* msg2 /* 0 */,
    const wchar_t* msg3 /* 0 */,
    const wchar_t* msg4 /* 0 */)
{
    _stderrLogger.OutputProgramErrorLine(msg1, msg2, msg3, msg4);
}

/// Output final results from all tests run.
void ConsoleLogger::OutputFinalResults(
    unsigned int succeeded, 
    unsigned int totalRun)
{
    if (succeeded != totalRun)
    {
        _stderrLogger.OutputFinalResults(succeeded, totalRun);
    }
    else
    {
        _stdoutLogger.OutputFinalResults(succeeded, totalRun);
    }
}

void ConsoleLogger::SetVerbosity(unsigned int verbosity)
{
    _stderrLogger.SetVerbosity(verbosity);
    _stdoutLogger.SetVerbosity(verbosity);
}
