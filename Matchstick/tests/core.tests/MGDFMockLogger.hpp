#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core { namespace tests {

class MockLogger: public ILogger {
public:
	MockLogger(){}
	virtual ~MockLogger(){}
	virtual void  SetLoggingLevel(LogLevel level){ }
	virtual LogLevel  GetLoggingLevel() const{ return LOG_ERROR; }
	virtual void  Add(const char *sender,const char *message,LogLevel level){}
};

}}}