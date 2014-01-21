#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <mutex>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFLogger.hpp>

namespace MGDF
{
namespace core
{

class ILoggerImpl: public ILogger
{
public:
	virtual void Flush() = 0;
	virtual void MoveOutputFile() = 0;
};

/**
 singleton event log, outputs to a file specified in constants.h
 uses buffered file writes to increase efficiency.
 \author gcconner
*/
class Logger: public ILoggerImpl
{
public:
	static Logger &Instance() {
		static Logger log;
		return log;
	}

	void SetLoggingLevel( LogLevel level ) override final;
	LogLevel GetLoggingLevel() const override final;
	void Add( const char * sender, const char * message, LogLevel level ) override final;

	void MoveOutputFile() override final;
	void Flush() override final;
private:
	Logger();
	virtual ~Logger();

	void SetOutputFile( const std::wstring & );

	std::mutex _mutex;
	std::vector<std::string> _events;
	std::wstring _filename;
	LogLevel _level;
};


#define LOG(msg,lvl) {\
	if (lvl <= MGDF::core::Logger::Instance().GetLoggingLevel()) {\
		std::ostringstream ss;\
		ss << __FILE__ <<  ':' <<__LINE__;\
		std::ostringstream ms;\
		ms << msg;\
		MGDF::core::Logger::Instance().Add(ss.str().c_str(),ms.str().c_str(),lvl);\
	}\
}

}
}
