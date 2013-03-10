#include "StdAfx.h"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#include "MGDFStatisticsManagerImpl.hpp"
#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

#define SEND_THRESHOLD 10

StatisticsManager::StatisticsManager()
{
	_saveBuffer = new std::vector<NameValuePair>();
	_statisticsFile = Resources::Instance().GameUserStatisticsFile();

	std::ofstream file( _statisticsFile.c_str(), std::ios_base::out | std::ios_base::trunc );
	file.close();
	LOG( "StatisticsManager enabled", LOG_LOW );
}

StatisticsManager::~StatisticsManager()
{
	if ( GetSaveBufferSize() > 0 ) {
		LOG( "Saving remaining statistics...", LOG_LOW );
		StatisticsManager::SaveAll();
	}
	SAFE_DELETE( _saveBuffer );
}

size_t StatisticsManager::GetSaveBufferSize()
{
	return _saveBuffer->size();
}

void StatisticsManager::AddToSaveBuffer( NameValuePair && nvp )
{
	_saveBuffer->push_back( nvp );
}

std::vector<StatisticsManager::NameValuePair> *StatisticsManager::ResetSaveBuffer()
{
	std::vector<NameValuePair> *result = _saveBuffer;
	_saveBuffer = new std::vector<NameValuePair>();
	return result;
}

void StatisticsManager::SaveStatistic( const char *  name, const char *  value )
{
	_ASSERTE( name );
	_ASSERTE( value );

	NameValuePair nvp;
	nvp.Name = name;
	nvp.Value = value;

	AddToSaveBuffer( std::move( nvp ) );

	if ( GetSaveBufferSize() >= SEND_THRESHOLD ) {
		StatisticsManager::SaveAll();
	}
}

void StatisticsManager::SaveAll()
{
	std::vector<NameValuePair> *saveBuffer = ResetSaveBuffer();

	if ( saveBuffer->size() > 0 ) {
		try {
			std::ofstream file( _statisticsFile.c_str(), std::ios_base::out | std::ios_base::app );
			BOOST_FOREACH( NameValuePair pair, ( *saveBuffer ) ) {
				file << pair.Name + " " << pair.Value << "\r\n";
			}
			file.close();
		} catch ( const std::exception& e ) {
			LOG( "Error saving statistics: " << e.what(), LOG_ERROR );
		}
	}
	SAFE_DELETE( saveBuffer );
}

}
}