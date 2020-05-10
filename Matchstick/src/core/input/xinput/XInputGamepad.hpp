#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#include <MGDF/MGDF.hpp>

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

class XInputGamepad : public IGamepad {
 public:
  XInputGamepad(INT32 id);
  virtual ~XInputGamepad(void){};

  bool IsConnected() const override final;
  bool IsButtonDown(GamepadButton button) const override final;
  bool IsButtonUp(GamepadButton button) const override final;
  bool IsButtonPress(GamepadButton button) override final;
  INT32 GetLeftTrigger() const override final;
  INT32 GetRightTrigger() const override final;
  INT32 GetLeftThumbX() const override final;
  INT32 GetLeftThumbY() const override final;
  INT32 GetRightThumbX() const override final;
  INT32 GetRightThumbY() const override final;
  void SetVibrationSpeed(INT32 left, INT32 right) override final;
  void GetState();

 private:
  INT32 _id, _frameIndex, _checkIndex;
  bool _connected;
  UCHAR _buttonpressState[32768];
  XINPUT_STATE _state;
};

}  // namespace xinput
}  // namespace input
}  // namespace core
}  // namespace MGDF