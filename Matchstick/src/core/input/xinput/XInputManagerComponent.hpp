#pragma once

#include <MGDF/MGDF.hpp>
#include <boost/thread/mutex.hpp>

#include "../../common/MGDFListImpl.hpp"
#include "../MGDFInputManagerComponent.hpp"

namespace MGDF { namespace core { namespace input { namespace xinput {

typedef ListImpl<IGamepadList,IGamepad *> GamepadList;

/**
 reference implementation of the inputmanager interface
 \author gcconner
*/
class XInputManagerComponent: public IInputManagerComponent
{
public:
	XInputManagerComponent();
	virtual ~XInputManagerComponent(void);

	virtual void HandleInput(RAWINPUT *input);
	virtual void HandleInput(INT32 mouseX,INT32 mouseY);
	virtual void ProcessSim();
	virtual void ProcessInput();

	virtual void ShowCursor(bool show);
	virtual bool  IsKeyDown(UINT16 key) const;
	virtual bool  IsKeyUp(UINT16 key) const;
	virtual bool  IsKeyPress(UINT16 key) const;
	virtual INT32  GetMouseX(void) const;
	virtual INT32  GetMouseY(void) const;
	virtual INT32  GetMouseDX(void) const;
	virtual INT32  GetMouseDY(void) const;
	virtual INT16  GetMouseDZ(void) const;
	virtual bool  IsButtonDown(Mouse mouseButton) const;
	virtual bool  IsButtonUp(Mouse mouseButton) const;
	virtual bool  IsButtonClicked(Mouse mouseButton);
	virtual const IGamepadList *  GetGamepads() const;

private:
	boost::mutex _simMutex;
	boost::mutex _inputMutex;

	//pending keyboard state (input thread)
	UINT8 _pendingKeyDown[256];
	UINT8 _pendingKeyDownEvents[256];
	UINT8 _pendingKeyDownEventsLength;
	UINT8 _pendingKeyPressEvents[256];
	UINT8 _pendingKeyPressEventsLength;
	
	bool _pendingShowCursor;
	bool _showCursor;

	//current keyboard state (sim thread)
	bool  _keyDown[256];
	bool  _keyPress[256];

	//pending mouse state (input thread)
	INT32 _pendingMouseDX, _pendingMouseDY;
	INT16 _pendingMouseDZ;
	UINT8 _pendingMouseButtonDown[3];
	bool _pendingMouseButtonClick[3];
	INT32 _pendingMouseX,_pendingMouseY;

	//current mouse state (sim thread)
	INT32 _mouseDX, _mouseDY;
	INT16 _mouseDZ;
	bool _mouseButtonDown[3];
	bool _mouseButtonClick[3];
	INT32 _mouseX, _mouseY;

	GamepadList _gamepads;
};

IInputManagerComponent *CreateXInputManagerComponent();

}}}}