#pragma once

#include <mutex>

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
	static MGDFError TryCreate( UINT32 maxFps, FrameLimiter **limiter );
	~FrameLimiter( void );

	/**
	limit the Fps to maxFps
	*/
	void LimitFps();

	double ProgressThroughCurrentFrame();

private:
	FrameLimiter( UINT32 maxFps );
	MGDFError Init();
	LARGE_INTEGER _freq, _previousFrameEnd;
	INT64 _frameTime;
	UINT32 _maxFps;

	std::mutex _frameEndMutex;
};

}
}
