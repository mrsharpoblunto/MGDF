#pragma once

#include <MGDF/MGDFTimer.hpp>

namespace MGDF { namespace core {

/**
this class is used for timing
*/
class Timer: public ITimer
{
public:
	Timer();
	virtual ~Timer(void);


	virtual LARGE_INTEGER GetCurrentTimeTicks();
	virtual LARGE_INTEGER GetTimerFrequency();
	virtual double ConvertDifferenceToSeconds(LARGE_INTEGER newTime,LARGE_INTEGER oldTime);

private:
	LARGE_INTEGER _freq;
};

}}
