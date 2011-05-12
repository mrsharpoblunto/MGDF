#include "StdAfx.h"

#include <math.h>
#include <mmsystem.h>
#include "MGDFTimer.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFExceptions.hpp"

namespace MGDF { namespace core {

Timer::Timer()
{
	timeBeginPeriod(1);//set a higher resolution for timing calls

    // exit if the system does not support a high performance timer
	if (!QueryPerformanceFrequency(&_freq)) {
		GetLoggerImpl()->Add(THIS_NAME,"High performance timer unsupported",LOG_ERROR);
		throw MGDFException("High performance timer unsupported");
	}
}

Timer::~Timer(void)
{
	timeEndPeriod(1);
}

LARGE_INTEGER Timer::GetCurrentTimeTicks()
{
	LARGE_INTEGER result;
	QueryPerformanceCounter(&result);
	return result;
}

LARGE_INTEGER Timer::GetTimerFrequency()
{
	return _freq;
}

double Timer::ConvertDifferenceToSeconds(LARGE_INTEGER newTime,LARGE_INTEGER oldTime)
{
	LONGLONG diff = newTime.QuadPart - oldTime.QuadPart;
	return (double)diff/_freq.QuadPart;
}


}}