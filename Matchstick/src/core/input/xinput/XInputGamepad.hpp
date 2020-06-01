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

  bool IsConnected() const final;
  bool IsButtonDown(GamepadButton button) const final;
  bool IsButtonUp(GamepadButton button) const final;
  bool IsButtonPress(GamepadButton button) final;
  INT32 GetLeftTrigger() const final;
  INT32 GetRightTrigger() const final;
  INT32 GetLeftThumbX() const final;
  INT32 GetLeftThumbY() const final;
  INT32 GetRightThumbX() const final;
  INT32 GetRightThumbY() const final;
  void SetVibrationSpeed(UINT16 left, UINT16 right) final;
  void UpdateState(const XINPUT_STATE &state);
  INT32 GetID() const { return _id; }
  void SetIsConnected(bool isConnected) { _connected = isConnected; }

 private:
  INT32 _id;
  bool _connected;
  UCHAR _buttonpressState[32768];
  XINPUT_STATE _state;
};

}  // namespace xinput
}  // namespace input
}  // namespace core
}  // namespace MGDF