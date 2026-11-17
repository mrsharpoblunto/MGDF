#pragma once

#include <MGDF/MGDF.h>

#include <atomic>
#include <chrono>

#include "Module.hpp"

namespace MGDF {
namespace Test {

class VRTests : public TestModule {
 public:
  // eyeFrames counts the frames that the render thread drew to both eyes
  explicit VRTests(const std::atomic<UINT64> &eyeFrames)
      : _eyeFrames(eyeFrames) {}

  void Setup(IMGDFSimHost *host) final;

 private:
  void StartWait();
  bool Expired() const;
  void SetVR(BOOL enabled);
  TestStep Confirm(std::shared_ptr<TestState> &state, const char *failure);

  const std::atomic<UINT64> &_eyeFrames;
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFRenderSettingsManager> _settings;
  UINT64 _seen = 0;
  std::chrono::steady_clock::time_point _lastFrame, _deadline;
};

}  // namespace Test
}  // namespace MGDF
