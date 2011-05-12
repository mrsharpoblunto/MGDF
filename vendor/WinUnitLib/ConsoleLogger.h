/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file ConsoleLogger.h
#pragma once

#include "FileLogger.h"

namespace WinUnitLib
{
    /// This class is a specialization of FileLogger which outputs to stdout
    /// and stderr.
    /// @ingroup Loggers
    class ConsoleLogger : public Logger
    {
    private:
        FileLogger _stdoutLogger;
        FileLogger _stderrLogger;
    public:
        ConsoleLogger();
    public:
        /// Outputs the full path and just the name of the executable tests
        /// are being run from, before running the tests.
        virtual void OutputTestExecutableNamePreTest(
            const wchar_t* /* fullPath */, 
            const wchar_t* /* nameOfExecutable */);

        /// Outputs the full path and just the name of the executable tests
        /// are being run from, after running the tests.
        virtual void OutputTestExecutableNamePostTest(
            const wchar_t* /* fullPath */, 
            const wchar_t* /* nameOfExecutable */, 
            unsigned int /* succeeded */,       ///< Number of tests succeeded
            unsigned int /* totalRun */);   ///< Number of tests run

        /// Outputs the name of the test about to be run.
        virtual void OutputTestNamePreTest(const char* /* testName */);

        /// Outputs the name of the test after it has been run, and whether 
        /// or not it passed.
        virtual void OutputTestNamePostTest(
            const char* /* testName */, 
            bool /* passed */);

        /// Output a trace message.  This comes from any OutputDebugString
        /// calls that have been made in the test executable.
        virtual void OutputTrace(const wchar_t* message);

        /// Output a message from a failed test.
        virtual void OutputTestError(const char* testName,const wchar_t* message);

        /// Output a series of up to four strings (followed by a newline) that
        /// describe an unexpected occurrence in the program.  This could
        /// be implemented with an assert, if interactivity is desired.
        virtual void OutputProgramErrorLine(
            const wchar_t* msg1,
            const wchar_t* msg2 = 0,
            const wchar_t* msg3 = 0,
            const wchar_t* msg4 = 0);

        /// Output final results from all tests run.
        virtual void OutputFinalResults(
            unsigned int /* succeeded */, 
            unsigned int /* totalRun */);

        virtual void SetVerbosity(unsigned int verbosity);
    };
}
