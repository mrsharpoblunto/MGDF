#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

class XInputGamepad : public ComBase<IMGDFGamepad> {
 public:
  XInputGamepad(INT32 id, bool connected);
  virtual ~XInputGamepad(void){};

  BOOL __stdcall IsConnected() final;
  BOOL __stdcall IsButtonDown(MGDFGamepadButton button) final;
  BOOL __stdcall IsButtonUp(MGDFGamepadButton button) final;
  BOOL __stdcall IsButtonPress(MGDFGamepadButton button) final;
  UINT8 __stdcall GetLeftTrigger() final;
  UINT8 __stdcall GetRightTrigger() final;
  INT32 __stdcall GetLeftThumbX() final;
  INT32 __stdcall GetLeftThumbY() final;
  INT32 __stdcall GetRightThumbX() final;
  INT32 __stdcall GetRightThumbY() final;
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