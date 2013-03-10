#pragma once

namespace MGDF {

/**
 Represents a names performance counter that can be used to time the duration of
 events. The results of these counters are shown in the MGDF stats overlay (press alt-f12 to see in game)
 \author gconner
 */
class IPerformanceCounter {
public:
	/**
	 Gets the name of the counter
	 \return the name of the counter
	 */
	virtual const char *GetName() const=0;

	/**
	 Begin timing 
	 */
	virtual void Begin()=0;

	/**
	 End timing and record the duration between the time when begin was called
	 and when End was called
	 */
	virtual void End()=0;

	virtual void Dispose()=0;
};

}