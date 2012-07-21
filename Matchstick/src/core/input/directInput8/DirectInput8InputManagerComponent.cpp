#include "stdafx.h"

#include "../../common/MGDFExceptions.hpp"
#include "../../common/MGDFLoggerImpl.hpp"

#include "DirectInput8InputManagerComponent.hpp"
#include "XInputGamepad.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace input { namespace directInput8 {

IInputManagerComponent *CreateDirectInput8InputManagerComponent(HINSTANCE instance,HWND window)
{
	try {
		return new DirectInput8InputManagerComponent(instance,window);
	}
	catch (...)
	{
		return nullptr;
	}
}

void DirectInput8InputManagerComponent::Dispose()
{
	delete this;
}

DirectInput8InputManagerComponent::DirectInput8InputManagerComponent(HINSTANCE instance,HWND window)
{
	for (int i=0;i<4;++i) {
		_gamepads.Add(new XInputGamepad(i));
	}
	//create the directinput device
	if (FAILED(DirectInput8Create(GetModuleHandle(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&_lpdi, NULL)))
		throw new MGDFException("unable to create directInput device");

	//initialise the keyboard
	if (FAILED(_lpdi->CreateDevice(GUID_SysKeyboard, &_keyboard, nullptr)))
			throw MGDFException("unable to initialise keyboard");
	if (FAILED(_keyboard->SetDataFormat(&c_dfDIKeyboard)))
			throw MGDFException("unable to initialise keyboard");
	if (FAILED(_keyboard->SetCooperativeLevel(window, DISCL_BACKGROUND |
		DISCL_NONEXCLUSIVE)))
			throw MGDFException("unable to initialise keyboard");

	//initialise the mouse
	if (FAILED(_lpdi->CreateDevice(GUID_SysMouse, &_mouse, nullptr)))
		throw MGDFException("unable to initialise mouse");
	if (FAILED(_mouse->SetCooperativeLevel(window, DISCL_BACKGROUND |
		DISCL_NONEXCLUSIVE)))
		throw MGDFException("unable to initialise mouse");
	if (FAILED(_mouse->SetDataFormat(&c_dfDIMouse)))
		throw MGDFException("unable to initialise mouse");

	if (FAILED(_mouse->Acquire()) || FAILED(_keyboard->Acquire())) {
		throw MGDFException("unable to Acquire input devices");
	}

	//initialise the keyboard keypress buffer
	for (int i = 0; i < 256; i++)
		_keypressState[i] = 0;

	//initialise the mouse buttonpress buffer
	for (int i = 0; i < 4; i++)
		_mouseButtonState[i] = 0;

	_mouseX = 0L;
	_mouseY = 0L;
	_mouseZ = 0L;
}

DirectInput8InputManagerComponent::~DirectInput8InputManagerComponent(void)
{
	for (auto iter = _gamepads.Items()->begin();iter!=_gamepads.Items()->end();++iter) {
		delete (XInputGamepad *)(*iter);
	}

	_mouse->Unacquire();
	_mouse->Release();
	_mouse = nullptr;

	_keyboard->Unacquire();
	_keyboard->Release();
	_keyboard = nullptr;

	_lpdi->Release();
	_lpdi = nullptr;
}

void DirectInput8InputManagerComponent::ReadInputDevices()
{
	//read controller states
	for (auto iter = _gamepads.Items()->begin();iter!=_gamepads.Items()->end();++iter) {
		((XInputGamepad *)(*iter))->GetState();
	}

	//read the keyboard state
	int diState = _keyboard->GetDeviceState(sizeof(UCHAR[256]),(LPVOID)_keystate);
	if(diState == DIERR_INPUTLOST) {
		_keyboard->Acquire();
		_keyboard->GetDeviceState(sizeof(UCHAR[256]), (LPVOID)_keystate);
	}
	
	//read the mouse state
	diState = _mouse->GetDeviceState(sizeof(DIMOUSESTATE),(LPVOID)&_mouseState);
	if(diState == DIERR_INPUTLOST) {
		_mouse->Acquire();
		_mouse->GetDeviceState(sizeof(DIMOUSESTATE), (LPVOID)&_mouseState);
	}

	//update the mouse positions based on thew mouses movement since the last polling
	_mouseX += _mouseState.lX;
	_mouseY += _mouseState.lY;
	_mouseZ += _mouseState.lZ;
}

bool DirectInput8InputManagerComponent::IsKeyDown(Key key) const
{
	return ((_keystate[key] & 0x80) ? true : false);
}

bool DirectInput8InputManagerComponent::IsKeyUp(Key key) const
{
	return ((_keystate[key] & 0x80) ? false : true);
}

bool DirectInput8InputManagerComponent::IsKeyPress(Key key)
{

	if (IsKeyDown(key))
		_keypressState[key] = 1;

	if (_keypressState[key] == 1)
		if (IsKeyUp(key))
			_keypressState[key] = 2;

	if (_keypressState[key] == 2)
	{
		_keypressState[key] = 0;
		return true;
	}
	
	return false;
}

long DirectInput8InputManagerComponent::GetMouseX(void) const
{
	return _mouseX;
}

long DirectInput8InputManagerComponent::GetMouseY(void) const
{
	return _mouseY;
}

long DirectInput8InputManagerComponent::GetMouseZ(void) const
{
	return _mouseZ;
}

void DirectInput8InputManagerComponent::SetMouseX(long x)
{
	_mouseX=x;
}

void DirectInput8InputManagerComponent::SetMouseY(long y)
{
	_mouseY=y;
}

void DirectInput8InputManagerComponent::SetMouseZ(long z)
{
	_mouseZ=z;
}

long DirectInput8InputManagerComponent::GetMouseMovementX() const
{
	return _mouseState.lX;
}

long DirectInput8InputManagerComponent::GetMouseMovementY() const
{
	return _mouseState.lY;
}

long DirectInput8InputManagerComponent::GetMouseMovementZ() const
{
	return _mouseState.lZ;
}

bool DirectInput8InputManagerComponent::IsButtonDown(Mouse mouseButton) const
{
	return ((_mouseState.rgbButtons[mouseButton] & 0x80) ? true : false);
}

bool DirectInput8InputManagerComponent::IsButtonUp(Mouse mouseButton) const
{
	return ((_mouseState.rgbButtons[mouseButton] & 0x80) ? false : true);
}

bool DirectInput8InputManagerComponent::IsButtonClicked(Mouse mouseButton)
{
	if (IsButtonDown(mouseButton))
		_mouseButtonState[mouseButton] = 1;

	if (_mouseButtonState[mouseButton] == 1)
		if (IsButtonUp(mouseButton))
			_mouseButtonState[mouseButton] = 2;

	if (_mouseButtonState[mouseButton] == 2)
	{
		_mouseButtonState[mouseButton] = 0;
		return true;
	}

	return false;	
}

const IGamepadList *DirectInput8InputManagerComponent::GetGamepads() const
{
	return &_gamepads;
}


}}}}
