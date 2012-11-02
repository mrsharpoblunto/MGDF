#pragma once

#include "../common/MGDFComponents.hpp"

#ifdef USE_XINPUT
#include "xinput/XInputManagerComponent.hpp"
#endif

namespace MGDF { namespace core { namespace input {

#ifdef USE_XINPUT
#define CreateInputManagerComponentImpl xinput::CreateXInputManagerComponent
#endif



}}}
