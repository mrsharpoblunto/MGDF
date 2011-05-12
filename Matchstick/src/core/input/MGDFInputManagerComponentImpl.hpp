#pragma once

#include "../common/MGDFComponents.hpp"

#ifdef USE_DIRECTINPUT8_INPUT
#include "directInput8/DirectInput8InputManagerComponent.hpp"
#endif

namespace MGDF { namespace core { namespace input {

#ifdef USE_DIRECTINPUT8_INPUT
#define CreateInputManagerComponentImpl directInput8::CreateDirectInput8InputManagerComponent
#endif



}}}
