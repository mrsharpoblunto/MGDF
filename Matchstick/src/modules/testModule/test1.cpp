#include "StdAfx.h"

#include "Test1.hpp"

#include <MGDF/ComObject.hpp>
#include <array>
#include <fstream>

#include "Test2.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

Test1::~Test1(void) {}

Test1::Test1() : _waitingForGamepad(false) { _time.QuadPart = -1; }

TestModule *Test1::NextTestModule() { return new Test2(); }

void Test1::Setup(ISimHost *host) {
  host->GetInput(_input.Assign());
  ComArray<IGamepad> gamepads(_input->GetGamepadCount());
  _input->GetGamepads(gamepads.Data());
  _gamepad = gamepads[0];
  host->GetTimer(_timer.Assign());
  StepOnce([](auto host, auto state) {
    state->AddLine("InputManager Tests");
    state->AddLine("");
    state->AddLine("Press the [ENTER] key");
  })
      .Step([this](auto host, auto state) {
        return _input->IsKeyPress(VK_RETURN) ? TestStep::PASSED
                                             : TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine("Press and hold [UP ARROW] key for at least one second");
      })
      .Step([this](auto host, auto state) {
        if (_input->IsKeyDown(VK_UP)) {
          _time = _timer->GetCurrentTimeTicks();
          return TestStep::NEXT;
        } else {
          return TestStep::CONT;
        }
      })
      .Step([this](auto host, auto state) {
        if (_input->IsKeyDown(VK_UP)) {
          if (_timer->ConvertDifferenceToSeconds(_timer->GetCurrentTimeTicks(),
                                                 _time) > 1) {
            return TestStep::PASSED;
          }
        }
        return TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine("Now release the [UP ARROW] key");
      })
      .Step([this](auto host, auto state) {
        return _input->IsKeyUp(VK_UP) ? TestStep::PASSED : TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine("Now click the left mouse button");
      })
      .Step([this](auto host, auto state) {
        return _input->IsButtonClicked(MOUSE_LEFT) ? TestStep::PASSED
                                                   : TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine("Now move the mouse up");
      })
      .Step([this](auto host, auto state) {
        return _input->GetMouseDY() < 0 ? TestStep::PASSED : TestStep::CONT;
      })
      .Step([this](auto host, auto state) {
        if (_gamepad->IsConnected()) {
          state->AddLine("Xbox controller detected");
          return TestStep::NEXT;
        } else if (!_waitingForGamepad) {
          _waitingForGamepad = true;
          state->AddLine("Plug in an Xbox controller");
        }
        return TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine("Press the [A] button on controller 1");
      })
      .Step([this](auto host, auto state) {
        return _gamepad->IsButtonPress(GAMEPAD_A) ? TestStep::PASSED
                                                  : TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine("Pull the left trigger on controller 1");
      })
      .Step([this](auto host, auto state) {
        return _gamepad->GetLeftTrigger() == 255 ? TestStep::PASSED
                                                 : TestStep::CONT;
      })
      .StepOnce([](auto host, auto state) {
        state->AddLine(
            "Pull the right trigger on controller 1, press [A] if the "
            "controller "
            "vibrates, [B] if it does not.");
      })
      .Step([this](auto host, auto state) {
        _gamepad->SetVibrationSpeed(_gamepad->GetRightTrigger() * 257,
                                    _gamepad->GetRightTrigger() * 257);
        if (_gamepad->IsButtonPress(GAMEPAD_A)) {
          _gamepad->SetVibrationSpeed(0, 0);
          return TestStep::PASSED;
        } else if (_gamepad->IsButtonPress(GAMEPAD_B)) {
          _gamepad->SetVibrationSpeed(0, 0);
          return TestStep::FAILED;
        }
        return TestStep::CONT;
      });
}

}  // namespace Test
}  // namespace MGDF