#include "StdAfx.h"

#include "MGDFFrameLimiter.hpp"

#include <math.h>
#include <windows.h>

#include "common/MGDFLoggerImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

MGDFError FrameLimiter::TryCreate(UINT32 maxFps, FrameLimiter **limiter) {
  *limiter = new FrameLimiter(maxFps);
  MGDFError error = (*limiter)->Init();
  if (MGDF_OK != error) {
    delete *limiter;
    *limiter = nullptr;
  }
  return error;
}

FrameLimiter::FrameLimiter(UINT32 maxFps) : _maxFps(maxFps) {}

MGDFError FrameLimiter::Init() {
  // exit if the  does not support a high performance timer
  if (!QueryPerformanceFrequency(&_freq)) {
    LOG("High performance timer unsupported", LOG_ERROR);
    return MGDF_ERR_CPU_TIMER_UNSUPPORTED;
  }

  QueryPerformanceCounter(&_previousFrameEnd);
  _frameTime = (LONGLONG)_freq.QuadPart /
               _maxFps;  // set the frame diff in ticks for fps times per second
  double ft = static_cast<double>(_freq.QuadPart) /
              _maxFps;  // set the frame diff in ticks for fps times per second

  return MGDF_OK;
}

FrameLimiter::~FrameLimiter(void) {}

/**
pauses for an unspecified amount of time in order that approximately maxFps
occur each second code based on timing code from Ryan Geiss
http://www.geisswerks.com/ryan/FAQS/timing.html
*/
LARGE_INTEGER FrameLimiter::LimitFps() {
  LARGE_INTEGER currentTime;
  QueryPerformanceCounter(&currentTime);

  if (_previousFrameEnd.QuadPart != 0) {
    bool done = false;
    do {
      QueryPerformanceCounter(&currentTime);

      INT64 timePassed = currentTime.QuadPart - _previousFrameEnd.QuadPart;
      INT64 timeLeft = _frameTime - timePassed;

      if (currentTime.QuadPart < _previousFrameEnd.QuadPart)  // time wrap
        done = true;
      if (timePassed >= _frameTime) done = true;

      if (!done) {
        // if > 0.002s left, do Sleep(1), which will actually sleep some
        //   steady amount, probably 1-2 ms,
        //   and do so in a nice way (cpu meter drops; laptop battery spared).
        // otherwise, do a few Sleep(0)'s, which just give up the timeslice,
        //   but don't really save cpu or battery, but do pass a tiny
        //   amount of time.
        if (timeLeft > (int)_freq.QuadPart * 2 / 1000)
          Sleep(1);
        else
          for (INT32 i = 0; i < 10; i++)
            Sleep(0);  // causes thread to give up its timeslice
      }
    } while (!done);
  }

  return _previousFrameEnd = currentTime;
}

}  // namespace core
}  // namespace MGDF
