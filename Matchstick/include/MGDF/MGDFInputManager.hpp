#pragma once

#include <MGDF/MGDFList.hpp>

namespace MGDF {

/**
this is represents all the various mouse clicks that are recognised by the input
manager
*/
enum Mouse {MOUSE_LEFT,MOUSE_RIGHT,MOUSE_MIDDLE};

/**
this is represents all the various key presses that are recognised by the input
manager, these scan codes are the same as those used by directInput.
*/
enum Key {
KEY_ESCAPE=0x01,
KEY_1,
KEY_2,
KEY_3,
KEY_4,
KEY_5,
KEY_6,
KEY_7,
KEY_8,
KEY_9,
KEY_0,
KEY_MINUS,
KEY_EQUALS,
KEY_BACK,
KEY_TAB,
KEY_Q,
KEY_W,
KEY_,
KEY_R,
KEY_T,
KEY_Y,
KEY_U,
KEY_I,
KEY_O,
KEY_P,
KEY_LBRACKET,
KEY_RBRACKET,
KEY_RETURN,
KEY_LCONTROL,
KEY_A,
KEY_S,
KEY_D,
KEY_F,
KEY_G,
KEY_H,
KEY_J,
KEY_K,
KEY_L,
KEY_SEMICOLON,
KEY_APOSTROPHE,
KEY_GRAVE,
KEY_LSHIFT,
KEY_BACKSLASH,
KEY_Z,
KEY_X,
KEY_C,
KEY_V,
KEY_B,
KEY_N,
KEY_M,
KEY_COMMA,
KEY_PERIOD,
KEY_SLASH,
KEY_RSHIFT,
KEY_MULTIPLY,
KEY_LMENU,
KEY_SPACE,
KEY_CAPITAL,
KEY_F1,
KEY_F2,
KEY_F3,
KEY_F4,
KEY_F5,
KEY_F6,
KEY_F7,
KEY_F8,
KEY_F9,
KEY_F10,
KEY_NUMLOCK,
KEY_SCROLL,
KEY_NUMPAD7,
KEY_NUMPAD8,
KEY_NUMPAD9,
KEY_SUBTRACT,
KEY_NUMPAD4,
KEY_NUMPAD5,
KEY_NUMPAD6,
KEY_ADD,
KEY_NUMPAD1,
KEY_NUMPAD2,
KEY_NUMPAD3,
KEY_NUMPAD0,
KEY_DECIMAL,
KEY_OEM_102,
KEY_F11,
KEY_F12,
KEY_F13=0x64,
KEY_F14,
KEY_F15,
KEY_KANA=0x70,
KEY_ABNT_C1=0x73,
KEY_CONVERT=0x79,
KEY_NOCONVERT=0x7B,
KEY_YEN=0x7D,
KEY_ABNT_C2,
KEY_NUMPADEQUALS=0x8D,
KEY_PREVTRACK=0x90,
KEY_AT,
KEY_COLON,
KEY_UNDERLINE,
KEY_KANJI,
KEY_STOP,
KEY_AX,
KEY_UNLABELED,
KEY_NEXTTRACK=0x99,
KEY_NUMPADENTER=0x9C,
KEY_RCONTROL,
KEY_MUTE=0xA0,
KEY_CALCULATOR,
KEY_PLAYPAUSE,
KEY_MEDIASTOP=0xA4,
KEY_VOLUMEDOWN=0xAE,
KEY_VOLUMEUP=0xB0,
KEY_WEBHOME=0xB2,
KEY_NUMPADCOMMA,
KEY_DIVIDE=0xB5,
KEY_SYSRQ=0xB7,
KEY_RMENU,
KEY_PAUSE=0xC5,
KEY_HOME=0xC7,
KEY_UP,
KEY_PRIOR,
KEY_LEFT=0xCB,
KEY_RIGHT=0xCD,
KEY_END= 0xCF,
KEY_DOWN=0xD0,
KEY_NEXT,
KEY_INSERT,
KEY_DELETE,
KEY_LWIN=0xDB,
KEY_RWIN,
KEY_APPS,
KEY_POWER,
KEY_SLEEP,
KEY_WAKE=0xE3,
KEY_WEBSEARCH=0xE5,
KEY_WEBFAVORITES,
KEY_WEBREFRESH,
KEY_WEBSTOP,
KEY_WEBFORWARD,
KEY_WEBBACK,
KEY_MYCOMPUTER,
KEY_MAIL,
KEY_MEDIASELECT 
};

enum GamepadButton
{
GAMEPAD_DPAD_UP =0x00000001,
GAMEPAD_DPAD_DOWN =0x00000002,
GAMEPAD_DPAD_LEFT=0x00000004,
GAMEPAD_DPAD_RIGHT=0x00000008,
GAMEPAD_START=0x00000010,
GAMEPAD_BACK=0x00000020,
GAMEPAD_LEFT_THUMB= 0x00000040,
GAMEPAD_RIGHT_THUMB=0x00000080,
GAMEPAD_LEFT_SHOULDER=0x0100,
GAMEPAD_RIGHT_SHOULDER=0x0200,
GAMEPAD_A=0x1000,
GAMEPAD_B=0x2000,
GAMEPAD_X=0x4000,
GAMEPAD_Y=0x8000
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
	virtual bool  IsConnected() const=0;

	/**
	determines if a button is presed
	\param key the gamepad button code representing the desired button
	\return true if the button is being held down
	*/
	virtual bool  IsButtonDown(GamepadButton button) const=0;

	/**
	determines if a button is NOT pressed
	\param key the gamepad button code representing the desired button
	\return true if the button is not being pressed
	*/
	virtual bool  IsButtonUp(GamepadButton button) const=0;

	/**
	determines if a button press has occured
	\param key the gamepad button code representing the desired button
	\return true if the button has been pressed (i.e pressed down then released)
	*/
	virtual bool  IsButtonPress(GamepadButton button)=0;

	/**
	returns the state of the controllers left trigger
	\return 0 for idle to 255 for fully held
	*/
	virtual int  GetLeftTrigger() const=0;

	/**
	returns the state of the controllers right trigger
	\return 0 for idle to 255 for fully held
	*/
	virtual int  GetRightTrigger() const=0;

	/**
	returns the state of the left thumbstick on the X axis
	\return -3768 for far left to +3768 for far right
	*/
	virtual int  GetLeftThumbX() const=0;

	/**
	returns the state of the left thumbstick on the Y axis
	\return -3768 for top to +3768 for bottom
	*/
	virtual int  GetLeftThumbY() const=0;

	/**
	returns the state of the right thumbstick on the X axis
	\return -3768 for far left to +3768 for far right
	*/
	virtual int  GetRightThumbX() const=0;

	/**
	returns the state of the right thumbstick on the Y axis
	\return -3768 for top to +3768 for bottom
	*/
	virtual int  GetRightThumbY() const=0;

	/**
	sets the vibration intensity in the left and right vibrator motors of the controller, valid values are 0-65535
	*/
	virtual void  SetVibrationSpeed(int left,int right)=0;
};

DECLARE_LIST(IGamepadList,IGamepad *)

/**
this class provides a means for modules to access information regarding user input
the purpose of providing this interface is to abstract away the details of
initialisation and management of low level key and mouse routines. The module can
simply extract user input data without knowledge of the particular input implementation
\author gcconner
*/
class IInputManager {
public:

/**
determines if a key is presed
\param key the keyboard code representing the desired key
\return true if the key is being held down
*/
virtual bool  IsKeyDown(Key key) const=0;

/**
determines if a key is NOT pressed
\param key the keyboard code representing the desired key
\return true if the key is not being pressed
*/
virtual bool  IsKeyUp(Key key) const=0;

/**
determines if a keypress has occured
\param key the keyboard code representing the desired key
\return true if the key has been pressed (i.e pressed down then released)
*/
virtual bool  IsKeyPress(Key key)=0;

/**
get the x coordinate of the mouse
\return the x coordinate of the mouse
*/
virtual long  GetMouseX(void) const=0;

/**
get the y coordinate of the mouse
\return the y coordinate of the mouse
*/
virtual long  GetMouseY(void) const=0;

/**
get the position of the scroll wheel
\return the position of the scroll wheel
*/
virtual long  GetMouseZ(void) const=0;

/**
set the mouse x coordinate
\param x the x coordinate
*/
virtual void  SetMouseX(long x)=0;

/**
set the mouse y coordinate
\param y the y coordinate
*/
virtual void  SetMouseY(long y)=0;

/**
set the position of the scroll wheel
\param z the position of the scroll wheel
*/
virtual void  SetMouseZ(long z)=0;

/**
get the x movement of the mouse since the last check
\return the x movement of the mouse since the last check
*/
virtual long  GetMouseMovementX(void) const=0;
/**
get the y movement of the mouse since the last check
\return the y movement of the mouse since the last check
*/
virtual long  GetMouseMovementY(void) const=0;

/**
get the scroll wheel movement of the mouse since the last check
\return the scroll wheel movement of the mouse since the last check
*/
virtual long  GetMouseMovementZ(void) const=0;

/**
determine if a mouse button is down
\param mouseButton the mousebutton code representing the desired button
\return true if the button is held down
*/
virtual bool  IsButtonDown(Mouse mouseButton) const=0;

/**
determine if a mouse button is up
\param mouseButton the mousebutton code representing the desired button
\return true if the button is NOT held down
*/
virtual bool  IsButtonUp(Mouse mouseButton) const=0;

/**
determine if a mouse button was clicked
\param mouseButton the mousebutton code representing the desired button
\return true if the button has been clicked, i.e held down then released
*/
virtual bool  IsButtonClicked(Mouse mouseButton)=0;

/**
gets a list of all compatible gamepads connected to the system
\return a list of all compatible gamepads connected to the system
*/
virtual const  IGamepadList * GetGamepads() const=0;
};

}
