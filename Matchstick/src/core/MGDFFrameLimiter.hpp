#pragma once

#include <boost/thread.hpp>

namespace MGDF
{
namespace core
{

/**
this class is used for timing and frame limiting
*/
class FrameLimiter
{
public:
	FrameLimiter( UINT32 maxFps );
	~FrameLimiter( void );

	/**
	limit the Fps to maxFps
	*/
	void LimitFps();

	double ProgressThroughCurrentFrame();

private:
	LARGE_INTEGER _freq, _previousFrameEnd;
	INT64 _frameTime;
	UINT32 _maxFps;

	boost::mutex _frameEndMutex;
};

}
}
