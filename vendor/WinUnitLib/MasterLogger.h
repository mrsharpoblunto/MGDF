/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file MasterLogger.h
#pragma once

#include "Logger.h"

#include "DebugLogger.h"
#include "ConsoleLogger.h"

namespace WinUnitLib
{
    /// This class is a special "master logger"--it holds the default loggers
    /// as well as any other loggers that are added.  When the Output* methods
    /// are called, it calls them on all loggers in the chain.
    /// @remarks Though I've used references Logger everywhere else possible,
    /// here I've used a pointer since it's easier to manage a logger chain 
    /// that way.
    /// @ingroup Loggers
    class MasterLogger : public Logger
    {
    private:
        // Default loggers
        DebugLogger _debugLogger;       ///< Logger that calls OutputDebugString.
        ConsoleLogger _consoleLogger;   ///< Logger that writes to stdout/stderr.

        unsigned int _verbosity;        ///< Verbosity level for all loggers in collection.

    public:
        MasterLogger(
            bool writeToConsole, 
            bool writeToDebugger, 
            unsigned int verbosity);

        void AddLogger(Logger* newLogger);

    public:
        virtual void OutputTestExecutableNamePreTest(
            const wchar_t* fullPath, const wchar_t* nameOfExecutable);

        virtual void OutputTestExecutableNamePostTest(
            const wchar_t* fullPath, const wchar_t* nameOfExecutable, 
            unsigned int succeeded, unsigned int totalRun);

        virtual void OutputTestNamePreTest(
            const char* testName);

        virtual void OutputTestNamePostTest(
            const char* testName, bool passed);

        virtual void OutputTestError(const char* testName,const wchar_t* message);
        
        virtual void OutputProgramErrorLine(
            const wchar_t* msg1,
            const wchar_t* msg2 = NULL,
            const wchar_t* msg3 = NULL,
            const wchar_t* msg4 = NULL); 
        
        virtual void OutputTrace(const wchar_t* message);
        
        virtual void OutputFinalResults(unsigned int succeeded, unsigned int totalRun);

        virtual void SetVerbosity(unsigned int verbosity);
    };
}