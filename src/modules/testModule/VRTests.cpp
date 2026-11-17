#include "StdAfx.h"

#include "VRTests.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

using namespace std::chrono_literals;

void VRTests::StartWait() {
  _seen = _eyeFrames.load();
  _lastFrame = std::chrono::steady_clock::now();
  _deadline = _lastFrame + 30s;
}

bool VRTests::Expired() const {
  return std::chrono::steady_clock::now() > _deadline;
}

void VRTests::SetVR(BOOL enabled) {
  ComObject<IMGDFPendingRenderSettingsChange> change;
  _settings->CreatePendingSettingsChange(change.Assign());
  change->SetVR(enabled);
}

TestStep VRTests::Confirm(std::shared_ptr<TestState> &state,
                          const char *failure) {
  if (_input->IsKeyPress('Y')) return TestStep::PASSED;
  if (_input->IsKeyPress('N')) return state->Fail(failure);
  return TestStep::CONT;
}

void VRTests::Setup(IMGDFSimHost *host) {
  host->GetInput(_input.Assign());
  host->GetRenderSettings(_settings.Assign());

  StepOnce([this](auto state) {
    state->Text.AddLine("");
    state->Text.AddLine("Put on the headset. Waiting for both eyes");
    StartWait();
  })
      .Step([this](auto state) {
        if (_eyeFrames.load() > _seen) return TestStep::PASSED;
        return Expired() ? state->Fail("The host drew no VR eye frames")
                         : TestStep::CONT;
      })
      .StepOnce([this](auto state) {
        state->Text.AddLine("Turning VR off");
        SetVR(FALSE);
        StartWait();
      })
      .Step([this](auto state) {
        const auto now = std::chrono::steady_clock::now();
        if (const auto frames = _eyeFrames.load(); frames != _seen) {
          _seen = frames;
          _lastFrame = now;
        }
        // the session ends some frames after the setting changes
        if (!_settings->GetVR() && now - _lastFrame > 1s)
          return TestStep::PASSED;
        return Expired() ? state->Fail("VR eye frames did not stop")
                         : TestStep::CONT;
      })
      .StepOnce([this](auto state) {
        state->Text.AddLine("Turning VR on");
        SetVR(TRUE);
        StartWait();
      })
      .Step([this](auto state) {
        if (_eyeFrames.load() > _seen) return TestStep::PASSED;
        return Expired() ? state->Fail("VR eye frames did not resume")
                         : TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine(
            "Each eye shows one color that changes as you turn your head, "
            "with a green panel ahead");
        state->Text.AddLine("Press [Y/N] if this works correctly");
      })
      .Step([this](auto state) {
        return Confirm(state, "The headset image was wrong");
      })
      .StepOnce([](auto state) {
        state->Text.AddLine(
            "Pull each controller trigger. The controller vibrates");
        state->Text.AddLine("Press [Y/N] if this works correctly");
      })
      .Step([this](auto state) {
        return Confirm(state, "A controller did not vibrate");
      });
}

}  // namespace Test
}  // namespace MGDF
