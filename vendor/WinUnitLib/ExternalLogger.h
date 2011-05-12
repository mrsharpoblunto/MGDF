/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
/// @file ExternalLogger.h
#pragma once

#include "Logger.h"

#include <windows.h> // HMODULE

namespace WinUnitLib
{
    /// This class wraps a custom logger implementation (DLL) into a Logger
    /// class like built-in loggers.
    /// @ingroup Loggers
    class ExternalLogger : public Logger
    {
    private:
        HMODULE _hmodule;   ///< The HMODULE of the loaded DLL holding the logger implementation
    public:
        ExternalLogger();
        ~ExternalLogger();

        // Load logger given a string in the form of <dllName>[:<initializationString>].
        bool LoadLogger(const wchar_t* reference);
    private:
        // Load logger given a filename and an initialization string.
        bool LoadLogger(const wchar_t* file, const wchar_t* initializationString);

    public:
        virtual void OutputTestExecutableNamePreTest(
            const wchar_t* /* fullPath */, 
            const wchar_t* /* nameOfExecutable */);

        virtual void OutputTestExecutableNamePostTest(
            const wchar_t* /* fullPath */, 
            const wchar_t* /* nameOfExecutable */, 
            unsigned int /* succeeded */, 
            unsigned int /* totalRun */);

        virtual void OutputTestNamePreTest(const char* /* testName */);

        virtual void OutputTestNamePostTest(
            const char* /* testName */, 
            bool /* passed */);

        virtual void OutputTrace(const wchar_t* message);

        virtual void OutputTestError(const char* testName,const wchar_t* message);

        virtual void OutputProgramErrorLine(
            const wchar_t* msg1,
            const wchar_t* msg2 = 0,
            const wchar_t* msg3 = 0,
            const wchar_t* msg4 = 0); 

        virtual void OutputFinalResults(
            unsigned int /* succeeded */, 
            unsigned int /* totalRun */);

        virtual void SetVerbosity(
            unsigned int verbosity);
    };
}
