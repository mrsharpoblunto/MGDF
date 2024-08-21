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

HRESULT FrameLimiter::TryCreate(UINT32 maxFps,
                                std::unique_ptr<FrameLimiter> &limiter) {
  auto l = std::make_unique<FrameLimiter>(maxFps);
  const auto result = l->Init();
  if (SUCCEEDED(result)) {
    limiter.swap(l);
  }
  return result;
}

FrameLimiter::FrameLimiter(UINT32 maxFps) : _maxFps(maxFps) {
  ::SecureZeroMemory(&_frameTime, sizeof(LARGE_INTEGER));
  ::SecureZeroMemory(&_freq, sizeof(LARGE_INTEGER));
  ::SecureZeroMemory(&_previousFrameEnd, sizeof(LARGE_INTEGER));
}

HRESULT FrameLimiter::Init() {
  // exit if the  does not support a high performance timer
  if (!::QueryPerformanceFrequency(&_freq)) {
    LOG("High performance timer unsupported", MGDF_LOG_ERROR);
    return E_FAIL;
  }

  ::QueryPerformanceCounter(&_previousFrameEnd);
  _frameTime = (LONGLONG)_freq.QuadPart /
               _maxFps;  // set the frame diff in ticks for fps times per second
  return S_OK;
}

FrameLimiter::~FrameLimiter(void) {}

/**
pauses for an unspecified amount of time in order that approximately maxFps
occur each second code based on timing code from Ryan Geiss
http://www.geisswerks.com/ryan/FAQS/timing.html
*/
LARGE_INTEGER FrameLimiter::LimitFps(bool &limitApplied) {
  LARGE_INTEGER currentTime;
  ::QueryPerformanceCounter(&currentTime);
  bool first = true;
  limitApplied = true;

  if (_previousFrameEnd.QuadPart != 0) {
    bool done = false;
    do {
      ::QueryPerformanceCounter(&currentTime);

      const INT64 timePassed =
          currentTime.QuadPart - _previousFrameEnd.QuadPart;
      const INT64 timeLeft = _frameTime - timePassed;

      if (currentTime.QuadPart < _previousFrameEnd.QuadPart)  // time wrap
        done = true;
      if (timePassed >= _frameTime) {
        done = true;
        if (first) {
          limitApplied = false;
        }
      }

      if (!done) {
        first = false;
        // if > 0.002s left, do Sleep(1), which will actually sleep some
        //   steady amount, probably 1-2 ms,
        //   and do so in a nice way (cpu meter drops; laptop battery spared).
        // otherwise, do a few Sleep(0)'s, which just give up the timeslice,
        //   but don't really save cpu or battery, but do pass a tiny
        //   amount of time.
        if (timeLeft > (int)_freq.QuadPart * 2 / 1000)
          ::Sleep(1);
        else
          for (INT32 i = 0; i < 10; i++)
            ::Sleep(0);  // causes thread to give up its timeslice
      }
    } while (!done);
  }

  return _previousFrameEnd = currentTime;
}

}  // namespace core
}  // namespace MGDF
