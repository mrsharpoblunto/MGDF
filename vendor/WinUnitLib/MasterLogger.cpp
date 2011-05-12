/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file MasterLogger.cpp

#include "MasterLogger.h"

#include "Logger.h"

namespace WinUnitLib
{
    /// Constructor sets up the two default loggers.
    MasterLogger::MasterLogger(
        bool writeToConsole,     ///< True if output should go to console 
        bool writeToDebugger,    ///< True if output should go to debug window
        unsigned int verbosity)  ///< Verbosity level for all loggers in chain
        :
    _verbosity(verbosity)
    {
        if (writeToConsole)
        {
            AddLogger(&_consoleLogger);
        }
        if (writeToDebugger)
        {
            AddLogger(&_debugLogger);
        }
    }

    void MasterLogger::OutputTestExecutableNamePreTest(
            const wchar_t* fullPath, const wchar_t* nameOfExecutable)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputTestExecutableNamePreTest(fullPath, nameOfExecutable);
        }
    }

    void MasterLogger::OutputTestExecutableNamePostTest(
            const wchar_t* fullPath, const wchar_t* nameOfExecutable,
            unsigned int succeeded, unsigned int totalRun)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputTestExecutableNamePostTest(fullPath, nameOfExecutable,
                succeeded, totalRun);
        }
    }

    void MasterLogger::OutputTestNamePreTest(const char* testName)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputTestNamePreTest(testName);
        }
    }

    void MasterLogger::OutputTestNamePostTest(const char* testName, bool passed)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputTestNamePostTest(testName, passed);
        }
    }

    void MasterLogger::OutputTrace(const wchar_t* message)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputTrace(message);
        }
    }

    void MasterLogger::OutputTestError(const char* testName,const wchar_t* message)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputTestError(testName,message);
        }
    }

    void MasterLogger::OutputProgramErrorLine(
            const wchar_t* msg1,
            const wchar_t* msg2,
            const wchar_t* msg3,
            const wchar_t* msg4)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputProgramErrorLine(msg1, msg2, msg3, msg4);
        }
    }

    void MasterLogger::OutputFinalResults(unsigned int succeeded, unsigned int totalRun)
    {
        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->OutputFinalResults(succeeded, totalRun);
        }
    }

    /// This function adds a logger to the logger chain, to be called by the
    /// MasterLogger::Output* functions.
    void MasterLogger::AddLogger(
        Logger* newLogger) ///< Logger to be added to chain
    {
        if (newLogger == NULL)
        {
            ::fwprintf(stderr, L"[stderr] MasterLogger::AddLogger: Attempted to add a NULL logger to chain.\n");
            return;
        }

        // Set the "next" of the incoming logger to the existing next, if any
        newLogger->Next = this->Next;

        // Set the existing next to the incoming logger
        this->Next = newLogger;

        // Set the verbosity level of the incoming logger
        newLogger->SetVerbosity(_verbosity);
    }

    void MasterLogger::SetVerbosity(unsigned int verbosity)
    {
        _verbosity = verbosity;

        for (Logger* logger = this->Next;
             logger != NULL;
             logger = logger->Next)
        {
            logger->SetVerbosity(verbosity);
        }
   }
}