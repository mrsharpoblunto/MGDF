/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file PlainTextLogger.h
#pragma once

#include <string>
#include "Logger.h"

using namespace std;

namespace WinUnitLib
{
    /// This abstract class implements the Logger functions for plain text output.  
    /// Classes that derive from it must implement operator<< for unsigned int,
    /// char*, and wchar_t*.
    /// @ingroup Loggers
    class PlainTextLogger : public Logger
    {
    private:
        unsigned int _verbosity;
    public:
        PlainTextLogger();
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

        virtual void OutputTestError(const char* /* testName */,const wchar_t* message);

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

    protected:
        virtual PlainTextLogger& operator<<(const wchar_t* str) = 0;
        virtual PlainTextLogger& operator<<(const char* str) = 0;
        virtual PlainTextLogger& operator<<(unsigned int i) = 0;
	private:
		wstring Escape(const wchar_t* s);
		string Escape(const char* s);
    };
}