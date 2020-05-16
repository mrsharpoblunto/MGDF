#pragma once

#include <MGDF/MGDFInputManager.hpp>

#include "../common/ComObject.hpp"
#include "../common/MGDFSystemComponent.hpp"

namespace MGDF {
namespace core {
namespace input {

class IInputManagerComponent : public ISystemComponent,
                               public ComBase<IInputManager> {
 public:
  virtual ~IInputManagerComponent() {}
  virtual void HandleInput(INT32 mouseX,
                           INT32 mouseY) = 0;  // deal with a mouse move event
  virtual void HandleInput(RAWINPUT *input) = 0;  // deal with a raw input event
  virtual void
  ClearInput() = 0;  // clear any pending input state (like key presses)
  virtual void ProcessSim() = 0;  // do any reading/processing of input required
                                  // before the current sim frame
  virtual void
  ProcessInput() = 0;  // do any processing required on the main input thread
};

}  // namespace input
}  // namespace core
}  // namespace MGDF
