#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>

#include <MGDF/MGDF.hpp>

namespace MGDF
{
namespace core
{
namespace input
{
namespace xinput
{

class XInputGamepad: public IGamepad
{
public:
	XInputGamepad( INT32 id );
	virtual ~XInputGamepad( void ) {};

	bool IsConnected() const override;
	bool IsButtonDown( GamepadButton button ) const override;
	bool IsButtonUp( GamepadButton button ) const override;
	bool IsButtonPress( GamepadButton button ) override;
	INT32 GetLeftTrigger() const override;
	INT32 GetRightTrigger() const override;
	INT32 GetLeftThumbX() const override;
	INT32 GetLeftThumbY() const override;
	INT32 GetRightThumbX() const override;
	INT32 GetRightThumbY() const override;
	void SetVibrationSpeed( INT32 left, INT32 right ) override;
	void GetState();
private:
	INT32 _id, _frameIndex, _checkIndex;
	bool _connected;
	UCHAR _buttonpressState[32768];
	XINPUT_STATE _state;
};

}
}
}
}