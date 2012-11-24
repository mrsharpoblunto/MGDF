#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core { namespace input { namespace xinput {

class XInputGamepad: public IGamepad
{
public:
	XInputGamepad(INT32 id);
	virtual ~XInputGamepad(void){};

	virtual bool IsConnected() const;
	virtual bool IsButtonDown(GamepadButton button) const;
	virtual bool IsButtonUp(GamepadButton button) const;
	virtual bool IsButtonPress(GamepadButton button);
	virtual INT32 GetLeftTrigger() const;
	virtual INT32 GetRightTrigger() const;
	virtual INT32 GetLeftThumbX() const;
	virtual INT32 GetLeftThumbY() const;
	virtual INT32 GetRightThumbX() const;
	virtual INT32 GetRightThumbY() const;
	virtual void SetVibrationSpeed(INT32 left,INT32 right);
	void GetState();
private:
	INT32 _id,_frameIndex,_checkIndex;
	bool _connected;
	UCHAR _buttonpressState[32768];
	XINPUT_STATE _state;
};

}}}}