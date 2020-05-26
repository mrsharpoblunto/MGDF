#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

class XInputGamepad : public ComBase<IGamepad> {
 public:
  XInputGamepad(INT32 id, bool connected);
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
  void UpdateState(const XINPUT_STATE &state);
  INT32 GetID() const { return _id; }
  void SetIsConnected(bool isConnected) { _connected = isConnected; }

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