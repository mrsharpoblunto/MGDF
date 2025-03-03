#pragma once

#include <MGDF/ComObject.hpp>

#include "../MGDFInputManagerComponent.hpp"

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

bool CreateInputManagerComponent(ComObject<IInputManagerComponent> &comp);

}  // namespace xinput
}  // namespace input
}  // namespace core
}  // namespace MGDF
