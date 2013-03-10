#pragma once

namespace MGDF
{

/**
defines the logging levels available, error is the highest level with log_high messages being of the
least importance
*/
enum LogLevel {LOG_ERROR, LOG_LOW, LOG_MEDIUM, LOG_HIGH};

/**
 this provides an interface to the engines logging functionality
 \author gcconner
 */
class ILogger
{
public:
	virtual void  SetLoggingLevel( LogLevel level ) = 0;
	virtual LogLevel  GetLoggingLevel() const = 0;
	virtual void  Add( const char *sender, const char *message, LogLevel level ) = 0;
};

}