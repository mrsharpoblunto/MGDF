#pragma once

#include <MGDF/MGDFPerformanceCounter.hpp>

namespace MGDF {


/**
 this provides an interface to the engines high resolution timer
 Unless specified, all methods are threadsafe and can be called from
 the render and sim threads
 \author gcconner
 */
class ITimer
{
public:
	virtual LARGE_INTEGER GetCurrentTimeTicks() const=0;
	virtual LARGE_INTEGER GetTimerFrequency() const=0;
	virtual double ConvertDifferenceToSeconds(LARGE_INTEGER newTime,LARGE_INTEGER oldTime) const=0;

	/**
	 * create a performance counter for profiling CPU time taken.
	 */
	virtual IPerformanceCounter *CreateCPUCounter(const char *name)=0;

	/**
	 * create a performance counter for profiling GPU time taken in DirectX API calls.
	 * NOTE: This method should ONLY be called from the render thread. Also GPU counters
	 * should only be used and removed from the render thread.
	 * \return NULL if GPU counters are unsupported (using D3D_FEATURE_LEVEL_9_3 or less) otherwise
	 * returns a new GPU timer instance.
	 */
	virtual IPerformanceCounter *CreateGPUCounter(const char *name)=0;
};

}