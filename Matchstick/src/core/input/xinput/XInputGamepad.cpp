#include "stdafx.h"

#include "XInputGamepad.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

XInputGamepad::XInputGamepad(INT32 id, bool connected)
    : _id(id), _connected(connected) {}

void XInputGamepad::UpdateState(const XINPUT_STATE &state) { _state = state; }

bool XInputGamepad::IsConnected() const { return _connected; }

bool XInputGamepad::IsButtonDown(GamepadButton button) const {
  return (_state.Gamepad.wButtons & button) == button;
}

bool XInputGamepad::IsButtonUp(GamepadButton button) const {
  return (_state.Gamepad.wButtons & button) != button;
}

bool XInputGamepad::IsButtonPress(GamepadButton button) {
  if (IsButtonDown(button)) _buttonpressState[button] = 1;

  if (_buttonpressState[button] == 1)
    if (IsButtonUp(button)) _buttonpressState[button] = 2;

  if (_buttonpressState[button] == 2) {
    _buttonpressState[button] = 0;
    return true;
  }

  return false;
}

INT32 XInputGamepad::GetLeftTrigger() const {
  return _state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
             ? _state.Gamepad.bLeftTrigger
             : 0;
}

INT32 XInputGamepad::GetRightTrigger() const {
  return _state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD
             ? _state.Gamepad.bRightTrigger
             : 0;
}

INT32 XInputGamepad::GetLeftThumbX() const {
  INT32 absX = abs(_state.Gamepad.sThumbLX);
  return absX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? _state.Gamepad.sThumbLX
                                                   : 0;
}

INT32 XInputGamepad::GetLeftThumbY() const {
  INT32 absY = abs(_state.Gamepad.sThumbLY);
  return absY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? _state.Gamepad.sThumbLY
                                                   : 0;
}

INT32 XInputGamepad::GetRightThumbX() const {
  INT32 absX = abs(_state.Gamepad.sThumbRX);
  return absX > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? _state.Gamepad.sThumbRX
                                                    : 0;
}

INT32 XInputGamepad::GetRightThumbY() const {
  INT32 absY = abs(_state.Gamepad.sThumbRY);
  return absY > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? _state.Gamepad.sThumbRY
                                                    : 0;
}

void XInputGamepad::SetVibrationSpeed(INT32 left, INT32 right) {
  if (_connected) {
    XINPUT_VIBRATION vibration;
    ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));
    vibration.wLeftMotorSpeed = left;    // use any value between 0-65535 here
    vibration.wRightMotorSpeed = right;  // use any value between 0-65535 here
    XInputSetState(_id, &vibration);
  }
}

}  // namespace xinput
}  // namespace input
}  // namespace core
}  // namespace MGDF