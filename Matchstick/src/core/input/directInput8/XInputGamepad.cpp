#include "stdafx.h"

#include "XInputGamepad.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

#define FRAMES_PER_CHECK 60

namespace MGDF { namespace core { namespace input { namespace directInput8 {

XInputGamepad::XInputGamepad(int id) {
	_id = id;
	_connected = false;
	_frameIndex = 0;
	_checkIndex = (FRAMES_PER_CHECK/4) * id;
}

void XInputGamepad::GetState()
{
	//polling disconnected controllers is EXTREMELY slow, so we only check once every 60 frames to see if a disconnected
	//controller has become connected, and we spread the checks for the 4 controllers across this 60 frame period to
	//minimize CPU spikes.
	if (_connected || _frameIndex==_checkIndex)
	{
		ZeroMemory( &_state, sizeof(XINPUT_STATE) );
		DWORD dwResult = XInputGetState( _id, &_state );

		if(dwResult == ERROR_SUCCESS)
		{ 
			_connected = true;
		}
		else
		{
			_connected = false;
		}
	}

	++_frameIndex;
	if (_frameIndex>=FRAMES_PER_CHECK) _frameIndex = 0;
}

bool XInputGamepad::IsConnected() const
{
	return _connected;
}

bool XInputGamepad::IsButtonDown(GamepadButton button) const {
	return (_state.Gamepad.wButtons & button) == button;
}


bool XInputGamepad::IsButtonUp(GamepadButton button ) const {
	return (_state.Gamepad.wButtons & button) != button;
}


bool XInputGamepad::IsButtonPress(GamepadButton button){
	if (IsButtonDown(button))
		_buttonpressState[button] = 1;

	if (_buttonpressState[button] == 1)
		if (IsButtonUp(button))
			_buttonpressState[button] = 2;

	if (_buttonpressState[button] == 2)
	{
		_buttonpressState[button] = 0;
		return true;
	}
	
	return false;
}


int XInputGamepad::GetLeftTrigger() const{
	return _state.Gamepad.bLeftTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? _state.Gamepad.bLeftTrigger : 0;
}


int XInputGamepad::GetRightTrigger() const{
	return _state.Gamepad.bRightTrigger>XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? _state.Gamepad.bRightTrigger : 0;
}


int XInputGamepad::GetLeftThumbX() const{
	return _state.Gamepad.sThumbLX> XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? _state.Gamepad.sThumbLX : 0;
}


int XInputGamepad::GetLeftThumbY() const{
	return _state.Gamepad.sThumbLY> XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ? _state.Gamepad.sThumbLY : 0;
}


int XInputGamepad::GetRightThumbX() const{
	return _state.Gamepad.sThumbRX> XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? _state.Gamepad.sThumbRX : 0;
}


int XInputGamepad::GetRightThumbY() const{
	return _state.Gamepad.sThumbRY> XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ? _state.Gamepad.sThumbRY : 0;
}


void XInputGamepad::SetVibrationSpeed(int left,int right){
	if (_connected) {
		XINPUT_VIBRATION vibration;
		ZeroMemory( &vibration, sizeof(XINPUT_VIBRATION) );
		vibration.wLeftMotorSpeed = left; // use any value between 0-65535 here
		vibration.wRightMotorSpeed = right; // use any value between 0-65535 here
		XInputSetState( _id, &vibration );
	}
}



}}}}