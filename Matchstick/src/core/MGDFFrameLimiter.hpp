#pragma once

#include <mutex>

namespace MGDF {
namespace core {

/**
this class is used for timing and frame limiting
*/
class FrameLimiter {
 public:
  static MGDFError TryCreate(UINT32 maxFps,
                             std::unique_ptr<FrameLimiter> &limiter);
  FrameLimiter(UINT32 maxFps);
  ~FrameLimiter(void);

  /**
  limit the Fps to maxFps
  */
  LARGE_INTEGER LimitFps();

 private:
  MGDFError Init();
  LARGE_INTEGER _freq, _previousFrameEnd;
  INT64 _frameTime;
  UINT32 _maxFps;
};

}  // namespace core
}  // namespace MGDF
