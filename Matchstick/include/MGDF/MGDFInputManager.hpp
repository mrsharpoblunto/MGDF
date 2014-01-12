#pragma once

#include <MGDF/MGDFList.hpp>

namespace MGDF
{

/**
this is represents all the various mouse clicks that are recognised by the input manager
*/
enum Mouse {MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE};

/**
All the gamepad button types recognized by the input manager
*/
enum GamepadButton {
    GAMEPAD_DPAD_UP = 0x00000001,
    GAMEPAD_DPAD_DOWN = 0x00000002,
    GAMEPAD_DPAD_LEFT = 0x00000004,
    GAMEPAD_DPAD_RIGHT = 0x00000008,
    GAMEPAD_START = 0x00000010,
    GAMEPAD_BACK = 0x00000020,
    GAMEPAD_LEFT_THUMB = 0x00000040,
    GAMEPAD_RIGHT_THUMB = 0x00000080,
    GAMEPAD_LEFT_SHOULDER = 0x0100,
    GAMEPAD_RIGHT_SHOULDER = 0x0200,
    GAMEPAD_A = 0x1000,
    GAMEPAD_B = 0x2000,
    GAMEPAD_X = 0x4000,
    GAMEPAD_Y = 0x8000
};

/**
this represents an xinput compatible gamepad connected to the PC
\author gcconner
*/
class IGamepad
{
public:
	/**
	determines if a controller is connected
	\return true if if a controller is connected
	*/
	virtual bool  IsConnected() const = 0;

	/**
	determines if a button is presed
	\param key the gamepad button code representing the desired button
	\return true if the button is being held down
	*/
	virtual bool  IsButtonDown( GamepadButton button ) const = 0;

	/**
	determines if a button is NOT pressed
	\param key the gamepad button code representing the desired button
	\return true if the button is not being pressed
	*/
	virtual bool  IsButtonUp( GamepadButton button ) const = 0;

	/**
	determines if a button press has occured
	\param key the gamepad button code representing the desired button
	\return true if the button has been pressed (i.e pressed down then released)
	*/
	virtual bool  IsButtonPress( GamepadButton button ) = 0;

	/**
	returns the state of the controllers left trigger
	\return 0 for idle to 255 for fully held
	*/
	virtual INT32  GetLeftTrigger() const = 0;

	/**
	returns the state of the controllers right trigger
	\return 0 for idle to 255 for fully held
	*/
	virtual INT32  GetRightTrigger() const = 0;

	/**
	returns the state of the left thumbstick on the X axis
	\return -32768 for far left to +32767 for far right
	*/
	virtual INT32  GetLeftThumbX() const = 0;

	/**
	returns the state of the left thumbstick on the Y axis
	\return -32768 for top to +32767 for bottom
	*/
	virtual INT32  GetLeftThumbY() const = 0;

	/**
	returns the state of the right thumbstick on the X axis
	\return -32768 for far left to +32767 for far right
	*/
	virtual INT32  GetRightThumbX() const = 0;

	/**
	returns the state of the right thumbstick on the Y axis
	\return -32768 for top to +32767 for bottom
	*/
	virtual INT32  GetRightThumbY() const = 0;

	/**
	sets the vibration intensity in the left and right vibrator motors of the controller, valid values are 0-65535
	*/
	virtual void  SetVibrationSpeed( INT32 left, INT32 right ) = 0;
};

DECLARE_LIST( IGamepadList, IGamepad * )

/**
this class provides a means for modules to access information regarding user input
from the mouse, keyboard, and xinput controllers
*/
class IInputManager
{
public:

	/**
	determines whether the OS cursor is displayed or not
	\param show Whether to show the cursor or not
	*/
	virtual void ShowCursor( bool show ) = 0;

	/**
	determines if a key is presed
	\param key the Microsoft Virtual key code representing the desired key (see here for a list http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx)
	\return true if the key is being held down
	*/
	virtual bool  IsKeyDown( UINT16 key ) const = 0;

	/**
	determines if a key is NOT pressed
	\param key the Microsoft Virtual key code representing the desired key (see here for a list http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx)
	\return true if the key is not being pressed
	*/
	virtual bool  IsKeyUp( UINT16 key ) const = 0;

	/**
	determines if a keypress has occured
	\param key the Microsoft Virtual key code representing the desired key (see here for a list http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx)
	\return true if the key has been pressed (i.e pressed down then released)
	*/
	virtual bool  IsKeyPress( UINT16 key ) const = 0;

	/**
	get the x position of the mouse
	\return the x position of the mouse relative to the left of the screen in pixels
	*/
	virtual INT32  GetMouseX( void ) const = 0;

	/**
	get the y position of the mouse
	\return the y position of the mouse relative to the top of the screen in pixels
	*/
	virtual INT32  GetMouseY( void ) const = 0;

	/**
	get the x movement of the mouse in the current frame
	\return the x movement of the mouse in the current frame
	*/
	virtual INT32  GetMouseDX( void ) const = 0;

	/**
	get the y movement of the mouse in the current frame
	\return the y movement of the mouse in the current frame
	*/
	virtual INT32  GetMouseDY( void ) const = 0;

	/**
	get the position of the scroll wheel
	\return the position of the scroll wheel, this value will be between -120 and 120
	*/
	virtual INT16 GetMouseDZ( void ) const = 0;

	/**
	determine if a mouse button is down
	\param mouseButton the mousebutton code representing the desired button
	\return true if the button is held down
	*/
	virtual bool  IsButtonDown( Mouse mouseButton ) const = 0;

	/**
	determine if a mouse button is up
	\param mouseButton the mousebutton code representing the desired button
	\return true if the button is NOT held down
	*/
	virtual bool  IsButtonUp( Mouse mouseButton ) const = 0;

	/**
	determine if a mouse button was clicked
	\param mouseButton the mousebutton code representing the desired button
	\return true if the button has been clicked, i.e held down then released
	*/
	virtual bool  IsButtonClicked( Mouse mouseButton ) = 0;

	/**
	gets a list of all compatible gamepads connected to the 
	\return a list of all compatible gamepads connected to the 
	*/
	virtual const  IGamepadList * GetGamepads() const = 0;
};

}
