#pragma once

#include <string>
#include <vector>
#include <boost/thread/thread.hpp>

#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFLogger.hpp>

namespace MGDF {
namespace core {

class ILoggerImpl: public ILogger
{
public:
	virtual void Flush()=0;
	virtual void Add(const std::string &sender,const std::string &message,LogLevel level)=0;
	virtual void Add(const std::string &sender,const std::string &message)=0;
	virtual void MoveOutputFile()=0;
};

/**
 singleton event log, outputs to a file specified in constants.h
 uses buffered file writes to increase efficiency.
 \author gcconner
*/
class Logger: public ILoggerImpl
{
public:
	static Logger *InstancePtr() {
		static Logger log;
		return &log;
	}

	virtual void SetLoggingLevel(LogLevel level);
	virtual LogLevel GetLoggingLevel() const;
	virtual void Add(const char * sender,const char * message,LogLevel level);
	virtual void Add(const std::string &sender,const std::string &message,LogLevel level);
	virtual void Add(const std::string &sender,const std::string &message);
	virtual void MoveOutputFile();

	virtual void Flush();
private:
	Logger();
	virtual ~Logger();

	void SetOutputFile(const std::wstring &);

	boost::mutex _mutex;
	std::vector<std::string> _events;
	std::wstring _filename;
	LogLevel _level;
};

MGDF_CORE_COMMON_DLL ILoggerImpl *GetLoggerImpl();

}
}
