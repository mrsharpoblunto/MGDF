/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file PlainTextLogger.cpp

#include "PlainTextLogger.h"

using namespace std;

namespace WinUnitLib
{
    PlainTextLogger::PlainTextLogger()
        :
    _verbosity(0)
    {
    }

    void PlainTextLogger::OutputTestExecutableNamePreTest(
        const wchar_t* /* fullPath */, 
        const wchar_t* nameOfExecutable)
    {
        if (_verbosity > 0)
        {
			*this << L"##teamcity[testSuiteStarted name='" << Escape(nameOfExecutable).c_str() << L"']\r\n";
        }
    }

    void PlainTextLogger::OutputTestExecutableNamePostTest(
        const wchar_t* /* fullPath */, 
        const wchar_t* nameOfExecutable, 
        unsigned int succeeded, 
        unsigned int totalRun)
    {
		*this << L"##teamcity[testSuiteFinished name='" << Escape(nameOfExecutable).c_str() << L"']\r\n";
        
		if (totalRun == 0)
        {
            *this << L"[" << nameOfExecutable << L"] " << 
                L"No tests were run.\n";
            return;
        }

        wchar_t* successString = (succeeded == totalRun) ? L"SUCCEEDED" : L"FAILED";
        if (succeeded != totalRun || _verbosity > 0)
        {
            *this << L"[" << nameOfExecutable << L"] " << successString 
                  << L".  Tests run: " << totalRun << L"; Failures: " 
                  << (totalRun - succeeded) << L".\n"; 
        }
    }

    void PlainTextLogger::OutputTestNamePreTest(const char* testName)
    {
        if (_verbosity > 0)
        {
			*this << L"##teamcity[testStarted name='" << Escape(testName).c_str() << L"']\r\n";
        }
    }

    void PlainTextLogger::OutputTestNamePostTest(
        const char* testName, 
        bool passed)
    {
		if (_verbosity > 0)
        {
			*this << L"##teamcity[testFinished name='" << Escape(testName).c_str() << L"']\r\n";
		}
    }

    void PlainTextLogger::OutputTrace(const wchar_t* message)
    {
        if (_verbosity > 0)
        {
            *this << message;
        }
    }

	wstring PlainTextLogger::Escape(const wchar_t* s) {
		wstring result;
    
		for (int i = 0; i < wcslen(s); i++) {
			wchar_t c = s[i];
	        
			switch (c) {
			case L'\n': result.append(L"|n"); break;
			case L'\r': result.append(L"|r"); break;
			case L'\'': result.append(L"|'"); break;
			case L'|':  result.append(L"||"); break;
			case L']':  result.append(L"|]"); break;
			default:   result.append(&c, 1);
			}
		}
	    
		return result;
	}

	string PlainTextLogger::Escape(const char* s) {
		string result;
    
		for (int i = 0; i < strlen(s); i++) {
			char c = s[i];
	        
			switch (c) {
			case '\n': result.append("|n"); break;
			case '\r': result.append("|r"); break;
			case '\'': result.append("|'"); break;
			case '|':  result.append("||"); break;
			case ']':  result.append("|]"); break;
			default:   result.append(&c, 1);
			}
		}
	    
		return result;
	}

    void PlainTextLogger::OutputTestError(const char* testName,const wchar_t* message)
    {
		*this << L"##teamcity[testFailed name='" << Escape(testName).c_str() << L"' message='test failed' details='" << Escape(message).c_str() << L"' ]\r\n" << message << L"\r\n";
    }

    void PlainTextLogger::OutputProgramErrorLine(
        const wchar_t* msg1,
        const wchar_t* msg2,
        const wchar_t* msg3,
        const wchar_t* msg4)
    {
        *this << L"[WinUnit] ";

        if (msg1) { *this << msg1; }
        if (msg2) { *this << msg2; }
        if (msg3) { *this << msg3; }
        if (msg4) { *this << msg4; }

        *this << L"\n";
    }

    void PlainTextLogger::OutputFinalResults(
        unsigned int succeeded, 
        unsigned int totalRun)
    {
        *this << L"\n";
        if (totalRun == 0)
        {
            *this << L"No tests were found.\n";
            return;
        }
        else if (succeeded == totalRun)
        {
            *this << L"All tests passed.\n";
        }
        else
        {
            *this << L"There were errors.\n";
        }
        *this << L"Tests run: " << totalRun 
            << L"; Failed: " << (totalRun - succeeded) << L".\n";
    }

    void PlainTextLogger::SetVerbosity(unsigned int verbosity)
    {
        _verbosity = verbosity;
    }
}