#include "StdAfx.h"

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#include "MGDFStatisticsManagerImpl.hpp"
#include <boost/foreach.hpp>
#include <iostream>
#include <fstream>

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core {

#define SEND_THRESHOLD 10

StatisticsManager::StatisticsManager(std::string gameUid)
{
	_saveBuffer = new std::vector<NameValuePair>();
	_statisticsFile = Resources::Instance().GameUserStatisticsFile(gameUid);

	std::ofstream file(_statisticsFile.c_str(),std::ios_base::out|std::ios_base::trunc);
	file.close();
	GetLoggerImpl()->Add(THIS_NAME,"StatisticsManager enabled");
}

StatisticsManager::~StatisticsManager()
{
	if (GetSaveBufferSize() > 0) {
		GetLoggerImpl()->Add(THIS_NAME,"Saving remaining statistics...");
		StatisticsManager::SaveAll();
	}
	SAFE_DELETE(_saveBuffer);
}

int StatisticsManager::GetSaveBufferSize() 
{
	return _saveBuffer->size();
}

void StatisticsManager::AddToSaveBuffer(NameValuePair nvp) 
{
	_saveBuffer->push_back(nvp);
}

std::vector<StatisticsManager::NameValuePair> *StatisticsManager::ResetSaveBuffer() 
{
	std::vector<NameValuePair> *result = _saveBuffer;
	_saveBuffer = new std::vector<NameValuePair>();
	return result;
}

void StatisticsManager::SaveStatistic(const char *  name,const char *  value)
{
	NameValuePair nvp;
	nvp.Name = name;
	nvp.Value = value;
	AddToSaveBuffer(nvp);

	if (GetSaveBufferSize()>= SEND_THRESHOLD) {
		StatisticsManager::SaveAll();
	}
}

void StatisticsManager::SaveAll() 
{
	std::vector<NameValuePair> *saveBuffer = ResetSaveBuffer();

	if (saveBuffer->size()>0) {
		try 
		{
			std::ofstream file(_statisticsFile.c_str(),std::ios_base::out|std::ios_base::app);
			BOOST_FOREACH(NameValuePair pair,(*saveBuffer)) 
			{
				file << pair.Name + " " << pair.Value << "\r\n";
			}
			file.close();
		}
		catch (std::exception& e)
		{
			GetLoggerImpl()->Add(THIS_NAME,std::string("Error saving statistics: ") + e.what());
		}
	}
	SAFE_DELETE(saveBuffer);
}

}}