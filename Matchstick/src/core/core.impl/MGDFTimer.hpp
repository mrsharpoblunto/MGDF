#pragma once

namespace MGDF { namespace core {

/**
this class is used for timing and frame limiting
*/
class Timer
{
public:
	Timer();
	virtual ~Timer(void);


	LARGE_INTEGER GetCurrentTimeTicks();
	LARGE_INTEGER GetTimerFrequency();
	double ConvertDifferenceToSeconds(LARGE_INTEGER newTime,LARGE_INTEGER oldTime);

private:
	LARGE_INTEGER _freq;
};

}}
