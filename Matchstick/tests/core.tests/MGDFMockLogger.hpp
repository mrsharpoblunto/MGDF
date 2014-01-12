#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF
{
namespace core
{
namespace tests
{

class MockLogger: public ILogger
{
public:
	MockLogger() {}
	virtual ~MockLogger() {}
	void  SetLoggingLevel( LogLevel level ) override { }
	LogLevel GetLoggingLevel() const override {
		return LOG_ERROR;
	}
	void Add( const char *sender, const char *message, LogLevel level ) override {}
};

}
}
}