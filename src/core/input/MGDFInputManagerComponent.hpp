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
  UINT64 __stdcall GetVRControllerCount() final { return _controllers.size(); }
  void __stdcall GetVRControllers(IMGDFVRController **controllers) final {
    if (!controllers) return;
    for (auto &controller : _controllers) controller.AddRawRef(controllers++);
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
  std::array<ComObject<VRController>, 2> _controllers{
      MakeCom<VRController>(_vr, MGDF_VR_HAND_LEFT),
      MakeCom<VRController>(_vr, MGDF_VR_HAND_RIGHT)};
};

}  // namespace input
}  // namespace core
}  // namespace MGDF
