#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <functional>
#include <memory>

#include "../core.impl/MGDFVRInput.hpp"

namespace MGDF {
namespace core {
namespace input {

class IInputManagerComponent : public ComBase<IMGDFInputManager> {
 public:
  virtual ~IInputManagerComponent() {}
  std::shared_ptr<VRInputState> GetVRInputState() const { return _vr; }
  BOOL __stdcall GetVRController(MGDFVRHand hand,
                                 MGDFVRController *controller) final {
    return _vr->GetController(hand, controller);
  }
  BOOL __stdcall VibrateVRController(MGDFVRHand hand, FLOAT amplitude,
                                     FLOAT seconds) final {
    return _vr->QueueHaptic(hand, amplitude, seconds, false);
  }
  BOOL __stdcall StopVRControllerVibration(MGDFVRHand hand) final {
    return _vr->QueueHaptic(hand, 0, 0, true);
  }
  BOOL __stdcall GetVRGaze(MGDFVRPose *pose) final {
    return _vr->GetGaze(pose);
  }
  virtual void HandleInput(INT32 mouseX,
                           INT32 mouseY) = 0;  // deal with a mouse move event
  virtual void HandleInput(
      std::function<RAWINPUT *()> getInput) = 0;  // deal with a raw input event
  virtual void
  ClearInput() = 0;  // clear any pending input state (like key presses)
  virtual void ProcessSim() = 0;  // do any reading/processing of input required
                                  // before the current sim frame
  virtual bool GetShowCursor() = 0;
  virtual MGDFCursor GetCursorShape() = 0;

 private:
  const std::shared_ptr<VRInputState> _vr = std::make_shared<VRInputState>();
};

}  // namespace input
}  // namespace core
}  // namespace MGDF
