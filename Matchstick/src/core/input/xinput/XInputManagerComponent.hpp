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
class XInputManagerComponent: public DisposeImpl<IInputManagerComponent>
{
public:
	XInputManagerComponent();
	virtual ~XInputManagerComponent(void);
	virtual void Dispose();

	virtual void HandleInput(RAWINPUT *input);
	virtual void ProcessInput();

	virtual bool  IsKeyDown(unsigned short key) const;
	virtual bool  IsKeyUp(unsigned short key) const;
	virtual bool  IsKeyPress(unsigned short key) const;
	virtual long  GetMouseDX(void) const;
	virtual long  GetMouseDY(void) const;
	virtual short  GetMouseDZ(void) const;
	virtual bool  IsButtonDown(Mouse mouseButton) const;
	virtual bool  IsButtonUp(Mouse mouseButton) const;
	virtual bool  IsButtonClicked(Mouse mouseButton);
	virtual const IGamepadList *  GetGamepads() const;

private:
	boost::mutex _mutex;

	//pending keyboard state (render thread)
	unsigned short _pendingKeyDown[256];
	unsigned short _pendingKeyDownEvents[256];
	unsigned short _pendingKeyDownEventsLength;
	unsigned short _pendingKeyPressEvents[256];
	unsigned short _pendingKeyPressEventsLength;

	//current keyboard state (sim thread)
	bool  _keyDown[256];
	bool  _keyPress[256];

	//pending mouse state (render thread)
	long _pendingMouseDX, _pendingMouseDY;
	short _pendingMouseDZ;
	unsigned short _pendingMouseButtonDown[3];
	bool _pendingMouseButtonClick[3];

	//current mouse state (sim thread)
	long _mouseDX, _mouseDY;
	short _mouseDZ;
	bool _mouseButtonDown[3];
	bool _mouseButtonClick[3];

	GamepadList _gamepads;
};

IInputManagerComponent *CreateXInputManagerComponent();

}}}}