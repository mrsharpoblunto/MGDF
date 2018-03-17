#include "stdafx.h"

#include <filesystem>
#include <sstream>
#include <fstream>
#include "MGDFLoggerImpl.hpp"
#include "MGDFResources.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

#define LOG_BUFFER_SIZE 10

void Logger::Add( const char *sender, const char *message, LogLevel level )
{
	_ASSERTE( sender );
	_ASSERTE( message );

	std::lock_guard<std::mutex> lock( _mutex );
	if ( level <= _level ) {
		std::ostringstream stream;
		stream << sender << " " << message << "\n";
		_events.push_back( stream.str() );
#if defined(_DEBUG)
		OutputDebugString( ("MGDF: "+_events.back()).c_str() );
#endif
		if ( _events.size() >= LOG_BUFFER_SIZE )
			Flush();
	}
}

void Logger::Flush()
{
	std::ofstream outFile;

	outFile.open( _filename.c_str(), std::ios::app );

	for ( std::string &evt : _events ) {
		outFile << evt;
	}
	_events.clear();

	outFile.close();
}

Logger::Logger()
{
	SetLoggingLevel( LOG_MEDIUM );
	SetOutputFile( Resources::Instance().LogFile() );
}

void Logger::MoveOutputFile()
{
	std::wstring newFile = Resources::Instance().LogFile();
	if ( newFile != _filename && _filename.size() && std::tr2::sys::exists( _filename )) {
		std::tr2::sys::path from( _filename );
		std::tr2::sys::path to( Resources::Instance().LogFile() );
		std::tr2::sys::copy_file( from, to, std::tr2::sys::copy_options::overwrite_existing );
		std::tr2::sys::remove( from );
	}
	_filename = newFile;
}


void Logger::SetOutputFile( const std::wstring &filename )
{
	_filename = filename;
	std::ofstream outFile;

	outFile.open( _filename.c_str(), std::ios::out );
	outFile.close();
}

void Logger::SetLoggingLevel( LogLevel level )
{
	std::lock_guard<std::mutex> lock( _mutex );
	_level = level;
}

LogLevel Logger::GetLoggingLevel() const
{
	return _level;
}

Logger::~Logger( void )
{
	Flush();
}

}
}
