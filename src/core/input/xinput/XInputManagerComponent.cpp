#include "stdafx.h"

#include "XInputManagerComponent.hpp"

#include "XInputManagerComponentImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

bool CreateInputManagerComponent(ComObject<IInputManagerComponent> &comp) {
  comp = ComObject<IInputManagerComponent>(new XInputManagerComponent());
  return true;
}

}  // namespace xinput
}  // namespace input
}  // namespace core
}  // namespace MGDF
