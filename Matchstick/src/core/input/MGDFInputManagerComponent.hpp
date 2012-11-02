#pragma once

#include "../common/MGDFSystemComponent.hpp" 
#include <MGDF/MGDFDisposable.hpp>
#include <MGDF/MGDFInputManager.hpp>

namespace MGDF { namespace core { namespace input {

class IInputManagerComponent: public ISystemComponent,public IInputManager,public IDisposable
{
public:
	virtual void HandleInput(RAWINPUT *input)=0;//deal with a raw input event
	virtual void ProcessInput()=0;//do any reading/processing of input required before the current frame 
};

}}}


