#include "stdafx.h"

#include <sstream>
#include <fstream>
#include "MGDFLoggerImpl.hpp"
#include "MGDFResources.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

ILoggerImpl *GetLoggerImpl()
{
	return Logger::InstancePtr();
}

#define LOG_BUFFER_SIZE 10

void Logger::Add(std::string sender,std::string message)
{
	Add(sender,message,LOG_MEDIUM);
}

void Logger::Add(std::string sender,std::string message,LogLevel level)
{
	boost::mutex::scoped_lock l(_mutex);
	if (level<=_level) {
		_events.push_back(sender+" "+message);
		if (_events.size()>=LOG_BUFFER_SIZE)
			Flush();
	}
}

void Logger::Add(const char *sender,const char *message,LogLevel level)
{
	boost::mutex::scoped_lock l(_mutex);
	if (level<=_level) {
		std::ostringstream stream;
		stream << sender << " " << message;
		_events.push_back(stream.str());
		if (_events.size()>=LOG_BUFFER_SIZE)
			Flush();
	}
}

void Logger::Flush()
{
	std::ofstream outFile;

	outFile.open(_filename.c_str(), std::ios::app);

	for (unsigned int i = 0; i < _events.size(); ++i) {
		outFile << _events[i] << "\n";
	}
	_events.clear();

	outFile.close();
}

Logger::Logger()
{
	SetLoggingLevel(LOG_MEDIUM);
	SetOutputFile(Resources::Instance().LogFile());
}

void Logger::SetOutputFile(std::string filename) 
{
	_filename = filename;
	std::ofstream outFile;

	outFile.open(_filename.c_str(), std::ios::out);
	outFile.close();
}

void Logger::SetLoggingLevel(LogLevel level)
{
	boost::mutex::scoped_lock l(_mutex);
	_level = level;
}

LogLevel Logger::GetLoggingLevel() const
{
	return _level;
}

Logger::~Logger(void)
{
	Flush();
}

}
}
