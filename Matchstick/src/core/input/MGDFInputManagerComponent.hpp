#pragma once

#include "../common/MGDFSystemComponent.hpp"
#include <MGDF/MGDFInputManager.hpp>

namespace MGDF
{
namespace core
{
namespace input
{

class IInputManagerComponent: public ISystemComponent, public IInputManager
{
public:
	virtual ~IInputManagerComponent() {}
	virtual void HandleInput( INT32 mouseX, INT32 mouseY ) = 0;  //deal with a mouse move event
	virtual void HandleInput( RAWINPUT *input ) = 0;  //deal with a raw input event
	virtual void ProcessSim() = 0; //do any reading/processing of input required before the current sim frame
	virtual void ProcessInput() = 0; //do any processing required on the main input thread
};

}
}
}


