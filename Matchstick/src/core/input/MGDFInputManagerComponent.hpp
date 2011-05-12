#pragma once

#include "../common/MGDFSystemComponent.hpp" 
#include <MGDF/MGDFDisposable.hpp>
#include <MGDF/MGDFInputManager.hpp>

namespace MGDF { namespace core { namespace input {

class IInputManagerComponent: public ISystemComponent,public IInputManager,public IDisposable
{
public:
	virtual void ReadInputDevices()=0;//read the state of the current input devices
};

}}}


