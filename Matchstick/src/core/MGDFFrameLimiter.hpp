#pragma once

#include <memory>
namespace MGDF {
namespace core {

/**
this class is used for timing and frame limiting
*/
class FrameLimiter {
 public:
  static HRESULT TryCreate(UINT32 maxFps,
                           std::unique_ptr<FrameLimiter> &limiter);
  FrameLimiter(UINT32 maxFps);
  ~FrameLimiter(void);

  /**
  limit the Fps to maxFps
  */
  LARGE_INTEGER LimitFps(bool &limitApplied);

 private:
  HRESULT Init();
  LARGE_INTEGER _freq, _previousFrameEnd;
  INT64 _frameTime;
  UINT32 _maxFps;
};

}  // namespace core
}  // namespace MGDF
