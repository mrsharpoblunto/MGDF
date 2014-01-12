#include "StdAfx.h"

#include <iostream>
#include <fstream>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#include "MGDFStatisticsManagerImpl.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

#define SEND_THRESHOLD 25

StatisticsManager::StatisticsManager()
{
	_statisticsFile = Resources::Instance().GameUserStatisticsFile();

	std::ofstream file( _statisticsFile.c_str(), std::ios_base::out | std::ios_base::trunc );
	file.close();
	LOG( "StatisticsManager enabled", LOG_LOW );
}

StatisticsManager::~StatisticsManager()
{
	if ( _saveBuffer.size() > 0 ) {
		LOG( "Saving remaining statistics...", LOG_LOW );
		SaveAll();
	}
}

void StatisticsManager::SaveStatistic( const char *  name, const char *  value )
{
	if ( !name || !value ) return;

	_saveBuffer.push_back(std::pair<std::string,std::string>(name,value));

	if ( _saveBuffer.size() >= SEND_THRESHOLD ) {
		SaveAll();
	}
}

void StatisticsManager::SaveAll()
{
	try 
	{
		std::ofstream file( _statisticsFile.c_str(), std::ios_base::out | std::ios_base::app );
		for ( auto &pair : _saveBuffer ) {
			file << pair.first + " " << pair.second << "\r\n";
		}
		file.close();
	} catch ( const std::exception& e ) {
		LOG( "Error saving statistics: " << e.what(), LOG_ERROR );
	}
	_saveBuffer.clear();
}

}
}