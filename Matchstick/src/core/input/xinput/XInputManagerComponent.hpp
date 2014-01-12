#pragma once

#include <MGDF/MGDF.hpp>
#include <mutex>

#include "../../common/MGDFListImpl.hpp"
#include "../MGDFInputManagerComponent.hpp"

namespace MGDF
{
namespace core
{
namespace input
{
namespace xinput
{

typedef ListImpl<IGamepadList, IGamepad *> GamepadList;

/**
 reference implementation of the inputmanager interface
 \author gcconner
*/
class XInputManagerComponent: public IInputManagerComponent
{
public:
	XInputManagerComponent();
	virtual ~XInputManagerComponent( void );

	void HandleInput( RAWINPUT *input ) override;
	void HandleInput( INT32 mouseX, INT32 mouseY ) override;
	void ProcessSim() override;
	void ProcessInput() override;

	void ShowCursor( bool show ) override;
	bool  IsKeyDown( UINT16 key ) const override;
	bool  IsKeyUp( UINT16 key ) const override;
	bool  IsKeyPress( UINT16 key ) const override;
	INT32  GetMouseX( void ) const override;
	INT32  GetMouseY( void ) const override;
	INT32  GetMouseDX( void ) const override;
	INT32  GetMouseDY( void ) const override;
	INT16  GetMouseDZ( void ) const override;
	bool  IsButtonDown( Mouse mouseButton ) const override;
	bool  IsButtonUp( Mouse mouseButton ) const override;
	bool  IsButtonClicked( Mouse mouseButton ) override;
	const IGamepadList *  GetGamepads() const override;

private:
	std::mutex _simMutex;
	std::mutex _inputMutex;

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
	INT32 _pendingMouseX, _pendingMouseY;

	//current mouse state (sim thread)
	INT32 _mouseDX, _mouseDY;
	INT16 _mouseDZ;
	bool _mouseButtonDown[3];
	bool _mouseButtonClick[3];
	INT32 _mouseX, _mouseY;

	GamepadList _gamepads;
};

IInputManagerComponent *CreateXInputManagerComponent();

}
}
}
}