#pragma once

namespace MGDF {

/**
 this provides an interface to the engines high resolution timer
 \author gcconner
 */
class ITimer
{
public:
	virtual LARGE_INTEGER GetCurrentTimeTicks()=0;
	virtual LARGE_INTEGER GetTimerFrequency()=0;
	virtual double ConvertDifferenceToSeconds(LARGE_INTEGER newTime,LARGE_INTEGER oldTime)=0;
};

}