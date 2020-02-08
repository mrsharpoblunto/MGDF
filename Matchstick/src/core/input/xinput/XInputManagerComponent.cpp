#include "stdafx.h"

#include "../../common/MGDFLoggerImpl.hpp"

#include "XInputManagerComponent.hpp"
#include "XInputGamepad.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{
namespace input
{
namespace xinput
{

IInputManagerComponent *CreateXInputManagerComponent()
{
	return new XInputManagerComponent();
}

XInputManagerComponent::XInputManagerComponent()
	: _pendingShowCursor(false)
	, _showCursor(false)
	, _pendingMouseX(0)
	, _pendingMouseY(0)
	, _pendingMouseDX(0L)
	, _pendingMouseDY(0L)
	, _pendingMouseDZ(0)
	, _mouseDX(0L)
	, _mouseDY(0L)
	, _mouseDZ(0)
	, _pendingKeyDownEventsLength(0)
	, _pendingKeyPressEventsLength(0)
{
	for (INT32 i = 0; i < XUSER_MAX_COUNT; ++i) {
		_gamepads.Add(new XInputGamepad(i));
	}
	ClearInput();
}

void XInputManagerComponent::ClearInput() 
{
	ZeroMemory( _pendingMouseButtonDown, sizeof( _pendingMouseButtonDown ) );
	ZeroMemory( _pendingMouseButtonClick, sizeof( _pendingMouseButtonClick ) );
	ZeroMemory( _mouseButtonDown, sizeof( _mouseButtonDown ) );
	ZeroMemory( _mouseButtonClick, sizeof( _mouseButtonClick ) );

	ZeroMemory( _pendingKeyDown, sizeof( _pendingKeyDown ) );
	ZeroMemory( _pendingKeyDownEvents, sizeof( _pendingKeyDownEvents ) );
	ZeroMemory( _pendingKeyPressEvents, sizeof( _pendingKeyPressEvents ) );
	ZeroMemory( _keyDown, sizeof( _keyDown ) );
	ZeroMemory( _keyPress, sizeof( _keyPress ) );
}

XInputManagerComponent::~XInputManagerComponent( void )
{
	for ( auto gamepad : *_gamepads.Items() ) {
		delete static_cast<XInputGamepad *>( gamepad );
	}
}

void XInputManagerComponent::HandleInput( INT32 mouseX, INT32 mouseY )
{
	std::lock_guard<std::mutex> lock( _simMutex );
	_pendingMouseX = mouseX;
	_pendingMouseY = mouseY;
}

void XInputManagerComponent::HandleInput( RAWINPUT *input )
{
	_ASSERTE( input );

	if ( input->header.dwType == RIM_TYPEKEYBOARD ) {
		std::lock_guard<std::mutex> lock( _simMutex );

		//we only know how to deal with keys
		if ( input->data.keyboard.VKey > UINT8_MAX ) return;

		UINT8 key = static_cast<UINT8>( input->data.keyboard.VKey );
		if ( ( input->data.keyboard.Flags & RI_KEY_MAKE ) == RI_KEY_MAKE && _pendingKeyDown[key] != 1 ) {
			_pendingKeyDown[key] = 1;
			_pendingKeyDownEvents[_pendingKeyDownEventsLength] = key;
			_pendingKeyDownEventsLength++;
		}
		if ( ( input->data.keyboard.Flags & RI_KEY_BREAK ) == RI_KEY_BREAK  && _pendingKeyDown[key] != 2 ) {
			_pendingKeyDown[key] = 2;
			_pendingKeyDownEvents[_pendingKeyDownEventsLength] = key;
			_pendingKeyDownEventsLength++;

			_pendingKeyPressEvents[_pendingKeyPressEventsLength] = key;
			_pendingKeyPressEventsLength++;
		}
	} else if ( input->header.dwType == RIM_TYPEMOUSE ) {
		std::lock_guard<std::mutex> lock( _simMutex );

		if ( ( input->data.mouse.usFlags & MOUSE_MOVE_RELATIVE ) == MOUSE_MOVE_RELATIVE ) {
			_pendingMouseDX += input->data.mouse.lLastX;
			_pendingMouseDY += input->data.mouse.lLastY;
		}

		//mouse scrollwheel
		if ( ( input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL ) == RI_MOUSE_WHEEL ) {
			_pendingMouseDZ += static_cast<INT16>( input->data.mouse.usButtonData );
		}
		//mouse button states
		if ( ( input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN ) == RI_MOUSE_LEFT_BUTTON_DOWN ) {
			_pendingMouseButtonDown[MOUSE_LEFT] = 1;
		}
		if ( ( input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP ) == RI_MOUSE_LEFT_BUTTON_UP ) {
			_pendingMouseButtonDown[MOUSE_LEFT] = 2;
			_pendingMouseButtonClick[MOUSE_LEFT] = true;
		}
		if ( ( input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN ) == RI_MOUSE_MIDDLE_BUTTON_DOWN ) {
			_pendingMouseButtonDown[MOUSE_MIDDLE] = 1;
		}
		if ( ( input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP ) == RI_MOUSE_MIDDLE_BUTTON_UP ) {
			_pendingMouseButtonDown[MOUSE_MIDDLE] = 2;
			_pendingMouseButtonClick[MOUSE_MIDDLE] = true;
		}
		if ( ( input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN ) == RI_MOUSE_RIGHT_BUTTON_DOWN ) {
			_pendingMouseButtonDown[MOUSE_RIGHT] = 1;
		}
		if ( ( input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP ) == RI_MOUSE_RIGHT_BUTTON_UP ) {
			_pendingMouseButtonDown[MOUSE_RIGHT] = 2;
			_pendingMouseButtonClick[MOUSE_RIGHT] = true;
		}
	}
}

void XInputManagerComponent::ProcessInput()
{
	if ( _pendingShowCursor ) {
		std::lock_guard<std::mutex> lock( _inputMutex );
		if ( _pendingShowCursor ) {
			_pendingShowCursor = false;
			::ShowCursor( _showCursor );
		}
	}
}

void XInputManagerComponent::ProcessSim()
{
	//handleinput occurs on a different thread to processinput so
	//we need to sync any access to the pending input state
	{
		std::lock_guard<std::mutex> lock( _simMutex );
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
		if ( _pendingMouseButtonDown[MOUSE_LEFT] ) {
			_mouseButtonDown[MOUSE_LEFT] = _pendingMouseButtonDown[MOUSE_LEFT] == 1;
		}
		if ( _pendingMouseButtonDown[MOUSE_MIDDLE] ) {
			_mouseButtonDown[MOUSE_MIDDLE] = _pendingMouseButtonDown[MOUSE_MIDDLE] == 1;
		}
		if ( _pendingMouseButtonDown[MOUSE_RIGHT] ) {
			_mouseButtonDown[MOUSE_RIGHT] = _pendingMouseButtonDown[MOUSE_RIGHT] == 1;
		}
		ZeroMemory( _pendingMouseButtonDown, sizeof( _pendingMouseButtonDown ) );

		//mouse clicks
		_mouseButtonClick[MOUSE_LEFT] = _pendingMouseButtonClick[MOUSE_LEFT];
		_mouseButtonClick[MOUSE_MIDDLE] = _pendingMouseButtonClick[MOUSE_MIDDLE];
		_mouseButtonClick[MOUSE_RIGHT] = _pendingMouseButtonClick[MOUSE_RIGHT];
		_pendingMouseButtonClick[MOUSE_LEFT] = false;
		_pendingMouseButtonClick[MOUSE_MIDDLE] = false;
		_pendingMouseButtonClick[MOUSE_RIGHT] = false;
		ZeroMemory( _pendingMouseButtonClick, sizeof( _pendingMouseButtonClick ) );


		//keyboard events
		for ( UINT8 i = 0; i < _pendingKeyDownEventsLength; ++i ) {
			UINT8 key = _pendingKeyDownEvents[i];
			_keyDown[key] = _pendingKeyDown[key] == 1;
		}
		ZeroMemory( _pendingKeyDown, sizeof( _pendingKeyDown ) );
		_pendingKeyDownEventsLength = 0;

		ZeroMemory( _keyPress, sizeof( _keyPress ) );
		for ( UINT8 i = 0; i < _pendingKeyPressEventsLength; ++i ) {
			UINT8 key = _pendingKeyPressEvents[i];
			_keyPress[key] = true;
		}
		_pendingKeyPressEventsLength = 0;
	}

	//read controller states
	for ( auto gamepad : *_gamepads.Items() ) {
		static_cast<XInputGamepad *>( gamepad )->GetState();
	}
}

void XInputManagerComponent::ShowCursor( bool show )
{
	std::lock_guard<std::mutex> lock( _inputMutex );
	_pendingShowCursor = true;
	_showCursor = show;
}

bool XInputManagerComponent::IsKeyDown( UINT16 key ) const
{
	return _keyDown[key];
}

bool XInputManagerComponent::IsKeyUp( UINT16 key ) const
{
	return !_keyDown[key];
}

bool XInputManagerComponent::IsKeyPress( UINT16 key ) const
{
	return _keyPress[key];
}

INT32 XInputManagerComponent::GetMouseX( void ) const
{
	return _mouseX;
}

INT32 XInputManagerComponent::GetMouseY( void ) const
{
	return _mouseY;
}

INT32 XInputManagerComponent::GetMouseDX() const
{
	return _mouseDX;
}

INT32 XInputManagerComponent::GetMouseDY() const
{
	return _mouseDY;
}

INT16 XInputManagerComponent::GetMouseDZ() const
{
	return _mouseDZ;
}

bool XInputManagerComponent::IsButtonDown( Mouse mouseButton ) const
{
	return _mouseButtonDown[mouseButton];
}

bool XInputManagerComponent::IsButtonUp( Mouse mouseButton ) const
{
	return !_mouseButtonDown[mouseButton];
}

bool XInputManagerComponent::IsButtonClicked( Mouse mouseButton )
{
	return _mouseButtonClick[mouseButton];
}

const IGamepadList *XInputManagerComponent::GetGamepads() const
{
	return &_gamepads;
}


}
}
}
}
