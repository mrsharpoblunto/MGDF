#include "stdafx.h"

#include "../../common/MGDFExceptions.hpp"
#include "../../common/MGDFLoggerImpl.hpp"

#include "XInputManagerComponent.hpp"
#include "XInputGamepad.hpp"

//this snippet ensures that the location of memory leaks is reported correctly in debug mode
#if defined(DEBUG) |defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF { namespace core { namespace input { namespace xinput {

IInputManagerComponent *CreateXInputManagerComponent()
{
	try {
		return new XInputManagerComponent();
	}
	catch (...)
	{
		return nullptr;
	}
}

void XInputManagerComponent::Dispose()
{
	delete this;
}

XInputManagerComponent::XInputManagerComponent()
{
	for (int i=0;i<4;++i) {
		_gamepads.Add(new XInputGamepad(i));
	}

	_pendingShowCursor = false;
	_showCursor = false;

	_pendingMouseX = 0;
	_pendingMouseY = 0;
	_pendingMouseDX = 0L;
	_pendingMouseDY = 0L;
	_pendingMouseDZ = 0;
	_mouseDX = 0L;
	_mouseDY = 0L;
	_mouseDZ = 0;
	ZeroMemory(_pendingMouseButtonDown,sizeof(_pendingMouseButtonDown));
	ZeroMemory(_pendingMouseButtonClick,sizeof(_pendingMouseButtonClick));
	ZeroMemory(_mouseButtonDown,sizeof(_mouseButtonDown));
	ZeroMemory(_mouseButtonClick,sizeof(_mouseButtonClick));

	ZeroMemory(_pendingKeyDown,sizeof(_pendingKeyDown));
	ZeroMemory(_pendingKeyDownEvents,sizeof(_pendingKeyDownEvents));
	_pendingKeyDownEventsLength = 0;
	ZeroMemory(_pendingKeyPressEvents,sizeof(_pendingKeyPressEvents));
	_pendingKeyPressEventsLength = 0;
	ZeroMemory(_keyDown,sizeof(_keyDown));
	ZeroMemory(_keyPress,sizeof(_keyPress));
}

XInputManagerComponent::~XInputManagerComponent(void)
{
	for (auto iter = _gamepads.Items()->begin();iter!=_gamepads.Items()->end();++iter) {
		delete (XInputGamepad *)(*iter);
	}
}

void XInputManagerComponent::HandleInput(int mouseX,int mouseY)
{
	boost::mutex::scoped_lock lock(_simMutex);
	_pendingMouseX = mouseX;
	_pendingMouseY = mouseY;
}

void XInputManagerComponent::HandleInput(RAWINPUT *input)
{
	if (input->header.dwType == RIM_TYPEKEYBOARD) 
	{
		boost::mutex::scoped_lock lock(_simMutex);

		unsigned short key = input->data.keyboard.VKey;
		if ((input->data.keyboard.Flags & RI_KEY_MAKE) == RI_KEY_MAKE && _pendingKeyDown[key]!=1)
		{
			_pendingKeyDown[key] = 1;
			_pendingKeyDownEvents[_pendingKeyDownEventsLength] = key;
			_pendingKeyDownEventsLength++;
		}
		if ((input->data.keyboard.Flags & RI_KEY_BREAK) == RI_KEY_BREAK  && _pendingKeyDown[key]!=2)
		{
			_pendingKeyDown[key] = 2;
			_pendingKeyDownEvents[_pendingKeyDownEventsLength] = key;
			_pendingKeyDownEventsLength++;

			_pendingKeyPressEvents[_pendingKeyPressEventsLength] = key;
			_pendingKeyPressEventsLength++;
		}
	}
	else if (input->header.dwType == RIM_TYPEMOUSE) 
	{
		boost::mutex::scoped_lock lock(_simMutex);

		if ((input->data.mouse.usFlags & MOUSE_MOVE_RELATIVE) == MOUSE_MOVE_RELATIVE)
		{
			_pendingMouseDX += input->data.mouse.lLastX;
			_pendingMouseDY += input->data.mouse.lLastY;
		}

		//mouse scrollwheel
		if ((input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL)
		{
			_pendingMouseDZ += (short)input->data.mouse.usButtonData;
		}
		//mouse button states
		if ((input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) == RI_MOUSE_LEFT_BUTTON_DOWN)
		{
			_pendingMouseButtonDown[MOUSE_LEFT] = 1;
		}
		if ((input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) == RI_MOUSE_LEFT_BUTTON_UP)
		{
			_pendingMouseButtonDown[MOUSE_LEFT] = 2;
			_pendingMouseButtonClick[MOUSE_LEFT] = true;
		}
		if ((input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) == RI_MOUSE_MIDDLE_BUTTON_DOWN)
		{
			_pendingMouseButtonDown[MOUSE_MIDDLE] = 1;
		}
		if ((input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) == RI_MOUSE_MIDDLE_BUTTON_UP)
		{
			_pendingMouseButtonDown[MOUSE_MIDDLE] = 2;
			_pendingMouseButtonClick[MOUSE_MIDDLE] = true;
		}
		if ((input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) == RI_MOUSE_RIGHT_BUTTON_DOWN)
		{
			_pendingMouseButtonDown[MOUSE_RIGHT] = 1;
		}
		if ((input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) == RI_MOUSE_RIGHT_BUTTON_UP)
		{
			_pendingMouseButtonDown[MOUSE_RIGHT] = 2;
			_pendingMouseButtonClick[MOUSE_RIGHT] = true;
		}
	} 
}

void XInputManagerComponent::ProcessInput()
{
	if (_pendingShowCursor)
	{
		boost::mutex::scoped_lock lock(_inputMutex);
		if (_pendingShowCursor)
		{
			_pendingShowCursor = false;
			::ShowCursor(_showCursor);
		}
	}
}

void XInputManagerComponent::ProcessSim()
{
	//handleinput occurs on a different thread to processinput so
	//we need to sync any access to the pending input state
	{
		boost::mutex::scoped_lock lock(_simMutex);
		//update keyboard and mouse state

		//mouse position
		_mouseX = _pendingMouseX;
		_mouseY = _pendingMouseY;

		//mouse movement
		_mouseDX = _pendingMouseDX;
		_mouseDY = _pendingMouseDY;
		_mouseDZ = _pendingMouseDZ;
		_pendingMouseDX = 0L;
		_pendingMouseDY = 0L;
		_pendingMouseDZ = 0;

		//mouse button states
		if (_pendingMouseButtonDown[MOUSE_LEFT])
		{
			_mouseButtonDown[MOUSE_LEFT] = _pendingMouseButtonDown[MOUSE_LEFT] == 1;
		}
		if (_pendingMouseButtonDown[MOUSE_MIDDLE])
		{
			_mouseButtonDown[MOUSE_MIDDLE] = _pendingMouseButtonDown[MOUSE_MIDDLE] == 1;
		}
		if (_pendingMouseButtonDown[MOUSE_RIGHT])
		{
			_mouseButtonDown[MOUSE_RIGHT] = _pendingMouseButtonDown[MOUSE_RIGHT] == 1;
		}
		ZeroMemory(_pendingMouseButtonDown,sizeof(_pendingMouseButtonDown));

		//mouse clicks
		_mouseButtonClick[MOUSE_LEFT] = _pendingMouseButtonClick[MOUSE_LEFT];
		_mouseButtonClick[MOUSE_MIDDLE] = _pendingMouseButtonClick[MOUSE_MIDDLE];
		_mouseButtonClick[MOUSE_RIGHT] = _pendingMouseButtonClick[MOUSE_RIGHT];
		_pendingMouseButtonClick[MOUSE_LEFT] = false;
		_pendingMouseButtonClick[MOUSE_MIDDLE] = false;
		_pendingMouseButtonClick[MOUSE_RIGHT] = false;
		ZeroMemory(_pendingMouseButtonClick,sizeof(_pendingMouseButtonClick));


		//keyboard events
		for (unsigned short i=0;i<_pendingKeyDownEventsLength;++i)
		{
			unsigned short key = _pendingKeyDownEvents[i];
			_keyDown[key] = _pendingKeyDown[key] == 1;
		}
		ZeroMemory(_pendingKeyDown,sizeof(_pendingKeyDown));
		_pendingKeyDownEventsLength = 0;

		ZeroMemory(_keyPress,sizeof(_keyPress));
		for (unsigned short i=0;i<_pendingKeyPressEventsLength;++i)
		{
			unsigned short key = _pendingKeyPressEvents[i];
			_keyPress[key] = true;
		}
		_pendingKeyPressEventsLength = 0;
	}

	//read controller states
	for (auto iter = _gamepads.Items()->begin();iter!=_gamepads.Items()->end();++iter) {
		((XInputGamepad *)(*iter))->GetState();
	}
}

void XInputManagerComponent::ShowCursor(bool show)
{
	boost::mutex::scoped_lock lock(_inputMutex);
	_pendingShowCursor = true;
	_showCursor = show;
}

bool XInputManagerComponent::IsKeyDown(unsigned short key) const
{
	return _keyDown[key];
}

bool XInputManagerComponent::IsKeyUp(unsigned short key) const
{
	return !_keyDown[key];
}

bool XInputManagerComponent::IsKeyPress(unsigned short key) const
{
	return _keyPress[key];
}

int XInputManagerComponent::GetMouseX(void) const
{
	return _mouseX;
}

int XInputManagerComponent::GetMouseY(void) const
{
	return _mouseY;
}

long XInputManagerComponent::GetMouseDX() const
{
	return _mouseDX;
}

long XInputManagerComponent::GetMouseDY() const
{
	return _mouseDY;
}

short XInputManagerComponent::GetMouseDZ() const
{
	return _mouseDZ;
}

bool XInputManagerComponent::IsButtonDown(Mouse mouseButton) const
{
	return _mouseButtonDown[mouseButton];
}

bool XInputManagerComponent::IsButtonUp(Mouse mouseButton) const
{
	return !_mouseButtonDown[mouseButton];
}

bool XInputManagerComponent::IsButtonClicked(Mouse mouseButton)
{
	return _mouseButtonClick[mouseButton];	
}

const IGamepadList *XInputManagerComponent::GetGamepads() const
{
	return &_gamepads;
}


}}}}
