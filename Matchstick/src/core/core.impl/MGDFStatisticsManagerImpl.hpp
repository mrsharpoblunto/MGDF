#pragma once

#include <MGDF/MGDFStatisticsManager.hpp>
#include <vector>

namespace MGDF { namespace core {

class StatisticsManager: public IStatisticsManager
{
public:
	StatisticsManager();
	virtual ~StatisticsManager();
	virtual void SaveStatistic(const char * name,const char *  value);
private:
	typedef struct {
		std::string Name;
		std::string Value;
	} NameValuePair;

	std::vector<NameValuePair> *_saveBuffer;
	std::wstring _statisticsFile;

	void SaveAll();
	size_t GetSaveBufferSize();
	void AddToSaveBuffer(NameValuePair &&nvp);
	std::vector<NameValuePair> *ResetSaveBuffer();
};

}}