#pragma once


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800 //use directinput 8
#include <dinput.h>

#include <MGDF/MGDF.hpp>
#include "../../common/MGDFListImpl.hpp"
#include "../MGDFInputManagerComponent.hpp"

namespace MGDF { namespace core { namespace input { namespace directInput8 {

typedef ListImpl<IGamepadList,IGamepad *> GamepadList;

/**
 reference implementation of the inputmanager interface
 \author gcconner
*/
class DirectInput8InputManagerComponent: public DisposeImpl<IInputManagerComponent>
{
public:
	DirectInput8InputManagerComponent(HINSTANCE instance,HWND window);
	virtual ~DirectInput8InputManagerComponent(void);
	virtual void Dispose();

	virtual void ReadInputDevices();//read the state of the current input devices

	virtual bool  IsKeyDown(Key key) const;
	virtual bool  IsKeyUp(Key key) const;
	virtual bool  IsKeyPress(Key key);
	virtual long  GetMouseX(void) const;
	virtual long  GetMouseY(void) const;
	virtual long  GetMouseZ(void) const;
	virtual void  SetMouseX(long);
	virtual void  SetMouseY(long);
	virtual void  SetMouseZ(long);
	virtual long  GetMouseMovementX(void) const;
	virtual long  GetMouseMovementY(void) const;
	virtual long  GetMouseMovementZ(void) const;
	virtual bool  IsButtonDown(Mouse mouseButton) const;
	virtual bool  IsButtonUp(Mouse mouseButton) const;
	virtual bool  IsButtonClicked(Mouse mouseButton);
	virtual const IGamepadList *  GetGamepads() const;

private:

	LPDIRECTINPUT _lpdi;
	LPDIRECTINPUTDEVICE _keyboard;
	UCHAR _keystate[256];
	UCHAR _keypressState[256];

	LPDIRECTINPUTDEVICE _mouse;
	long _mouseX, _mouseY,_mouseZ;
	DIMOUSESTATE _mouseState;
	BYTE _mouseButtonState[3];

	GamepadList _gamepads;
};

MGDF_CORE_DLL IInputManagerComponent *CreateDirectInput8InputManagerComponent(HINSTANCE instance,HWND window);

}}}}