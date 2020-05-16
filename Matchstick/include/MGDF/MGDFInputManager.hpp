#pragma once

namespace MGDF {

/**
this is represents all the various mouse buttons that are recognised by the
input manager
*/
enum Mouse { MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE };

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
Provides the ability to query the state of and control other properties of an
XInput compatible gamepad connected to the PC \author gcconner
*/
MIDL_INTERFACE("84C7C6BF-827E-4958-8C64-D7FA78D69D0B")
IGamepad : public IUnknown {
 public:
  /**
  determines if a controller is connected
  \return true if if a controller is connected
  */
  virtual bool STDMETHODCALLTYPE IsConnected() const = 0;

  /**
  determines if a button is pressed
  \param button the gamepad button code representing the desired button
  \return true if the button is being held down
  */
  virtual bool STDMETHODCALLTYPE IsButtonDown(GamepadButton button) const = 0;

  /**
  determines if a button is NOT pressed
  \param button the gamepad button code representing the desired button
  \return true if the button is not being pressed
  */
  virtual bool STDMETHODCALLTYPE IsButtonUp(GamepadButton button) const = 0;

  /**
  determines if a button press has occured
  \param button the gamepad button code representing the desired button
  \return true if the button has been pressed (i.e pressed down then released)
  */
  virtual bool STDMETHODCALLTYPE IsButtonPress(GamepadButton button) = 0;

  /**
  returns the state of the controllers left trigger
  \return 0 for idle to 255 for fully held
  */
  virtual INT32 STDMETHODCALLTYPE GetLeftTrigger() const = 0;

  /**
  returns the state of the controllers right trigger
  \return 0 for idle to 255 for fully held
  */
  virtual INT32 STDMETHODCALLTYPE GetRightTrigger() const = 0;

  /**
  returns the state of the left thumbstick on the X axis
  \return -32768 for far left to +32767 for far right
  */
  virtual INT32 STDMETHODCALLTYPE GetLeftThumbX() const = 0;

  /**
  returns the state of the left thumbstick on the Y axis
  \return -32768 for top to +32767 for bottom
  */
  virtual INT32 STDMETHODCALLTYPE GetLeftThumbY() const = 0;

  /**
  returns the state of the right thumbstick on the X axis
  \return -32768 for far left to +32767 for far right
  */
  virtual INT32 STDMETHODCALLTYPE GetRightThumbX() const = 0;

  /**
  returns the state of the right thumbstick on the Y axis
  \return -32768 for top to +32767 for bottom
  */
  virtual INT32 STDMETHODCALLTYPE GetRightThumbY() const = 0;

  /**
  sets the vibration intensity in the left and right vibrator motors of the
  controller, valid values are 0-65535 \param left the intensity of the left
  vibrator motor \param right the intensity of the right vibrator motor
  */
  virtual void STDMETHODCALLTYPE SetVibrationSpeed(INT32 left, INT32 right) = 0;
};

/**
Provides a means for modules to access information regarding user input
from the mouse, keyboard, and xinput controllers
*/
MIDL_INTERFACE("98EC499D-5359-4902-8BA9-6E36686E9898")
IInputManager : public IUnknown {
 public:
  /**
  determines whether the OS cursor is displayed or not
  \param show Whether to show the cursor or not
  */
  virtual void STDMETHODCALLTYPE ShowCursor(bool show) = 0;

  /**
  determines if a key is presed
  \param key the Microsoft Virtual key code representing the desired key (see
  here for a list
  http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx)
  \return true if the key is being held down
  */
  virtual bool STDMETHODCALLTYPE IsKeyDown(UINT16 key) const = 0;

  /**
  determines if a key is NOT pressed
  \param key the Microsoft Virtual key code representing the desired key (see
  here for a list
  http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx)
  \return true if the key is not being pressed
  */
  virtual bool STDMETHODCALLTYPE IsKeyUp(UINT16 key) const = 0;

  /**
  determines if a keypress has occured
  \param key the Microsoft Virtual key code representing the desired key (see
  here for a list
  http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx)
  \return true if the key has been pressed (i.e pressed down then released)
  */
  virtual bool STDMETHODCALLTYPE IsKeyPress(UINT16 key) const = 0;

  /**
  get the x position of the mouse
  \return the x position of the mouse relative to the left of the screen in
  pixels
  */
  virtual INT32 STDMETHODCALLTYPE GetMouseX(void) const = 0;

  /**
  get the y position of the mouse
  \return the y position of the mouse relative to the top of the screen in
  pixels
  */
  virtual INT32 STDMETHODCALLTYPE GetMouseY(void) const = 0;

  /**
  get the x movement of the mouse in the current frame
  \return the x movement of the mouse in the current frame
  */
  virtual INT32 STDMETHODCALLTYPE GetMouseDX(void) const = 0;

  /**
  get the y movement of the mouse in the current frame
  \return the y movement of the mouse in the current frame
  */
  virtual INT32 STDMETHODCALLTYPE GetMouseDY(void) const = 0;

  /**
  get the position of the scroll wheel
  \return the position of the scroll wheel, this value will be between -120 and
  120
  */
  virtual INT16 STDMETHODCALLTYPE GetMouseDZ(void) const = 0;

  /**
  determine if a mouse button is down
  \param mouseButton the mousebutton code representing the desired button
  \return true if the button is held down
  */
  virtual bool STDMETHODCALLTYPE IsButtonDown(Mouse mouseButton) const = 0;

  /**
  determine if a mouse button is up
  \param mouseButton the mousebutton code representing the desired button
  \return true if the button is NOT held down
  */
  virtual bool STDMETHODCALLTYPE IsButtonUp(Mouse mouseButton) const = 0;

  /**
  determine if a mouse button was clicked
  \param mouseButton the mousebutton code representing the desired button
  \return true if the button has been clicked, i.e held down then released
  */
  virtual bool STDMETHODCALLTYPE IsButtonClicked(Mouse mouseButton) = 0;

  /**
  gets a list of all compatible gamepads connected to the PC
  \param number the number of gamepads to retrieve
  \param gamepads pointer to an array of gamepads
   \return true if the supplied gamepad array is large enough to hold all the
  results, otherwise returns false and sets the size required in number
  */
  virtual bool STDMETHODCALLTYPE GetGamepads(UINT32 * number,
                                             IGamepad * *gamepads) = 0;
};

}  // namespace MGDF
