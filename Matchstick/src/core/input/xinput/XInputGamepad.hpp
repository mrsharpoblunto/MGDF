#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#include <MGDF/MGDF.hpp>

namespace MGDF { namespace core { namespace input { namespace xinput {

class XInputGamepad: public IGamepad
{
public:
	XInputGamepad(int id);
	virtual ~XInputGamepad(void){};

	virtual bool IsConnected() const;
	virtual bool IsButtonDown(GamepadButton button) const;
	virtual bool IsButtonUp(GamepadButton button) const;
	virtual bool IsButtonPress(GamepadButton button);
	virtual int GetLeftTrigger() const;
	virtual int GetRightTrigger() const;
	virtual int GetLeftThumbX() const;
	virtual int GetLeftThumbY() const;
	virtual int GetRightThumbX() const;
	virtual int GetRightThumbY() const;
	virtual void SetVibrationSpeed(int left,int right);
	void GetState();
private:
	int _id,_frameIndex,_checkIndex;
	bool _connected;
	UCHAR _buttonpressState[32768];
	XINPUT_STATE _state;
};

}}}}