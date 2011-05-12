#pragma once 

namespace MGDF {

class IStatisticsManager {
public:
	virtual void  SaveStatistic(const char *name,const char *value)=0;
};

}