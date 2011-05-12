#pragma once

#include <boost/thread.hpp>

namespace MGDF { namespace core {

/**
this class is used for timing and frame limiting
*/
class FrameLimiter
{
public:
	FrameLimiter(unsigned int maxFps);
	~FrameLimiter(void);

	/**
	limit the Fps to maxFps
	*/
	void LimitFps();

	double ProgressThroughCurrentFrame();

private:
	LARGE_INTEGER _freq,_previousFrameEnd;
	LONGLONG _frameTime;
	unsigned int _maxFps;

	boost::mutex _frameEndMutex;
};

}}
