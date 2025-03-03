#include "StdAfx.h"

#include "InputTests.hpp"

#include <MGDF/ComObject.hpp>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

InputTests::~InputTests(void) {}

InputTests::InputTests() : _waitingForGamepad(false) { _time.QuadPart = -1; }

void InputTests::Setup(IMGDFSimHost *host) {
  host->GetInput(_input.Assign());
  ComArray<IMGDFGamepad> gamepads(_input->GetGamepadCount());
  _input->GetGamepads(gamepads.Data());
  _gamepad = gamepads[0];
  host->GetTimer(_timer.Assign());
  StepOnce([](auto state) {
    state->Text.AddLine("");
    state->Text.AddLine("InputManager Tests");
    state->Text.AddLine("");
    state->Text.AddLine("Press the [ENTER] key");
  })
      .Step([this](auto state) {
        std::ignore = state;
        return _input->IsKeyPress(VK_RETURN) ? TestStep::PASSED
                                             : TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine(
            "Press and hold [UP ARROW] key for at least one second");
      })
      .Step([this](auto state) {
        std::ignore = state;
        if (_input->IsKeyDown(VK_UP)) {
          if (!_time.QuadPart) {
            _time = _timer->GetCurrentTimeTicks();
          }
          if (_timer->ConvertDifferenceToSeconds(_timer->GetCurrentTimeTicks(),
                                                 _time) > 1) {
            return TestStep::NEXT;
          } else {
            return TestStep::CONT;
          }
        } else {
          _time.QuadPart = 0;
          return TestStep::CONT;
        }
      })
      .StepOnce([](auto state) {
        state->Text.AddLine("Now release the [UP ARROW] key");
      })
      .Step([this](auto state) {
        std::ignore = state;
        return _input->IsKeyUp(VK_UP) ? TestStep::PASSED : TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine("Now click the left mouse button");
      })
      .Step([this](auto state) {
        std::ignore = state;
        return _input->IsButtonClicked(MGDF_MOUSE_LEFT) ? TestStep::PASSED
                                                        : TestStep::CONT;
      })
      .StepOnce(
          [](auto state) { state->Text.AddLine("Now move the mouse up"); })
      .Step([this](auto state) {
        std::ignore = state;
        return _input->GetMouseDY() < 0 ? TestStep::PASSED : TestStep::CONT;
      })
      .Step([this](auto state) {
        if (_gamepad->IsConnected()) {
          state->Text.AddLine("Xbox controller detected");
          return TestStep::NEXT;
        } else if (!_waitingForGamepad) {
          _waitingForGamepad = true;
          state->Text.AddLine("Plug in an Xbox controller");
        }
        return TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine("Press the [A] button on controller 1");
      })
      .Step([this](auto state) {
        std::ignore = state;
        return _gamepad->IsButtonPress(MGDF_GAMEPAD_A) ? TestStep::PASSED
                                                       : TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine("Pull the left trigger on controller 1");
      })
      .Step([this](auto state) {
        std::ignore = state;
        return _gamepad->GetLeftTrigger() == 255 ? TestStep::PASSED
                                                 : TestStep::CONT;
      })
      .StepOnce([](auto state) {
        state->Text.AddLine(
            "Pull the right trigger on controller 1, press [A] if the "
            "controller "
            "vibrates, [B] if it does not.");
      })
      .Step([this](auto state) {
        std::ignore = state;
        _gamepad->SetVibrationSpeed(
            static_cast<UINT16>(_gamepad->GetRightTrigger() * 257),
            static_cast<UINT16>(_gamepad->GetRightTrigger() * 257));
        if (_gamepad->IsButtonPress(MGDF_GAMEPAD_A)) {
          _gamepad->SetVibrationSpeed(0, 0);
          return TestStep::PASSED;
        } else if (_gamepad->IsButtonPress(MGDF_GAMEPAD_B)) {
          _gamepad->SetVibrationSpeed(0, 0);
          return TestStep::FAILED;
        }
        return TestStep::CONT;
      });
}

}  // namespace Test
}  // namespace MGDF