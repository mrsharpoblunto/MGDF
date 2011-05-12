/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file Logger.h
#pragma once

typedef unsigned long       DWORD;

#define WINUNITLIB_WIDEN2(x) L ## x
#define WINUNITLIB_WIDEN(x) WINUNITLIB_WIDEN2(x)
#ifndef __WFUNCTION__
#define __WFUNCTION__ WINUNITLIB_WIDEN(__FUNCTION__)
#endif 

/// @defgroup Loggers Loggers
///

namespace WinUnitLib
{
    /// This class is the base class for loggers that receive events that occur
    /// during the course of running tests.  Loggers can decide whether to 
    /// respond to the events and what to output.
    /// @ingroup Loggers
    class Logger
    {
    private:
        Logger* _next;              ///< Next logger in chain (NULL if none)
    public:
        Logger(void);
        virtual ~Logger(void) = 0 {}
        
    public:
        /// Outputs the full path and just the name of the executable tests
        /// are being run from, before running the tests.
        virtual void OutputTestExecutableNamePreTest(
            const wchar_t* /* fullPath */, 
            const wchar_t* /* nameOfExecutable */) = 0;

        /// Outputs the full path and just the name of the executable tests
        /// are being run from, after running the tests.
        virtual void OutputTestExecutableNamePostTest(
            const wchar_t* /* fullPath */, 
            const wchar_t* /* nameOfExecutable */, 
            unsigned int /* succeeded */,       ///< Number of tests succeeded
            unsigned int /* totalRun */) = 0;   ///< Number of tests run

        /// Outputs the name of the test about to be run.
        virtual void OutputTestNamePreTest(const char* /* testName */) = 0;

        /// Outputs the name of the test after it has been run, and whether 
        /// or not it passed.
        virtual void OutputTestNamePostTest(
            const char* /* testName */, 
            bool /* passed */) = 0;

        /// Output a trace message.  This comes from any OutputDebugString
        /// calls that have been made in the test executable.
        virtual void OutputTrace(const wchar_t* message) = 0;

        /// Output a message from a failed test.
        virtual void OutputTestError(const char* testName,const wchar_t* message) = 0;

        /// Output a series of up to four strings (followed by a newline) that
        /// describe an unexpected occurrence in the program.  This could
        /// be implemented with an assert, if interactivity is desired.
        virtual void OutputProgramErrorLine(
            const wchar_t* msg1,
            const wchar_t* msg2 = 0,
            const wchar_t* msg3 = 0,
            const wchar_t* msg4 = 0) = 0;

        /// Output final results from all tests run.
        virtual void OutputFinalResults(
            unsigned int /* succeeded */, 
            unsigned int /* totalRun */) = 0;

        /// Set verbosity level.
        virtual void SetVerbosity(unsigned int verbosity) = 0;

    public:
        /// The next logger in the chain.  (This is a virtual property that 
        /// uses GetNext() and SetNext().)
        __declspec(property (get=GetNext, put=SetNext)) Logger* Next;
        void SetNext(Logger* next);
        Logger* GetNext();

    public:
        /// Gets the system error message associated with the given error code
        /// and includes that in an OutputProgramErrorLine call.
        void OutputWinapiError(DWORD errorCode, wchar_t* callingFunction, 
            const wchar_t* msg1, const wchar_t* msg2 = 0); 
    };
}