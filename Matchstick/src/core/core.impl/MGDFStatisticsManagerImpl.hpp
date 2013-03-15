#pragma once

#include <MGDF/MGDFStatisticsManager.hpp>
#include <vector>

namespace MGDF
{
namespace core
{

class StatisticsManager: public IStatisticsManager
{
public:
	StatisticsManager();
	virtual ~StatisticsManager();
	virtual void SaveStatistic( const char * name, const char *  value );
private:
	std::vector<std::pair<std::string,std::string> > _saveBuffer;
	std::wstring _statisticsFile;
	void SaveAll();
};

}
}