#pragma once

namespace MGDF
{

/**
defines the logging levels available, error is the highest level with log_high messages being of the
least importance
*/
enum LogLevel {LOG_ERROR, LOG_LOW, LOG_MEDIUM, LOG_HIGH};

/**
 this provides an interface to the MGDF logging system
 */
class ILogger
{
public:
	/**
	 set the current logging verbosity level
	 \param level the verbosity level
	 */
	virtual void SetLoggingLevel( LogLevel level ) = 0;

	/**
	 get the current logging verbosity level
	 \return the current logging verbosity level
	 */
	virtual LogLevel GetLoggingLevel() const = 0;

	/**
	 write an entry to the log
	 \param sender the location or component writing the log message
	 \param message the content of the log message
	 \level the verbosity filter for writing out the message. The message will be written if this is less than or equal
	 to the current logging level
	 */
	virtual void Add( const char *sender, const char *message, LogLevel level ) = 0;
};

}