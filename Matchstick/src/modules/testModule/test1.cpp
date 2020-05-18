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

Test1::Test1() { _testState = 0; }

TestModule *Test1::NextTestModule() {
  if (_testState == 12) {
    return new Test2();
  } else {
    return nullptr;
  }
}

void Test1::Update(ISimHost *host, TextManagerState *state) {
  ComObject<IInputManager> input;
  host->GetInput(input.Assign());
  ComArray<IGamepad> gamepads(input->GetGamepadCount());
  input->GetGamepads(gamepads.Data());

  if (input->IsKeyPress(VK_ESCAPE)) {
    host->ShutDown();
  }

  if (_testState == 11) {
    gamepads[0]->SetVibrationSpeed(gamepads[0]->GetRightTrigger() * 257,
                                   gamepads[0]->GetRightTrigger() * 257);
  }

  time_t now = time(nullptr);

  if (_testState == 0) {
    _testState++;
    state->AddLine("InputManager Tests");
    state->AddLine("");
    state->AddLine("Press the [ENTER] key");
  } else if (_testState == 1 && input->IsKeyPress(VK_RETURN)) {
    _testState++;
    state->SetStatus(GREEN, "[Test Passed]");
    state->AddLine("Press and hold [UP ARROW] key for at least one second");
  } else if (_testState == 2 && input->IsKeyDown(VK_UP)) {
    _time = host->GetTimer()->GetCurrentTimeTicks();
    _testState++;
  } else if (_testState == 3) {
    if (input->IsKeyDown(VK_UP)) {
      if (host->GetTimer()->ConvertDifferenceToSeconds(
              host->GetTimer()->GetCurrentTimeTicks(), _time) > 1) {
        _testState++;
        state->SetStatus(GREEN, "[Test Passed]");
        state->AddLine("Now release the [UP ARROW] key");
      }
    } else {
      _testState--;
    }
  } else if (_testState == 4 && input->IsKeyUp(VK_UP)) {
    _testState++;
    state->SetStatus(GREEN, "[Test Passed]");
    state->AddLine("Now click the left mouse button");
  } else if (_testState == 5 && input->IsButtonClicked(MOUSE_LEFT)) {
    _testState++;
    state->SetStatus(GREEN, "[Test Passed]");
    state->AddLine("Now move the mouse up");
  } else if (_testState == 6 && input->GetMouseDY() < 0) {
    _testState++;
    state->SetStatus(GREEN, "[Test Passed]");
    if (gamepads[0]->IsConnected()) {
      _testState++;
      state->AddLine("Xbox 360 controller detected");
    } else {
      state->AddLine(
          "Plug in an xbox 360 controller or press [S] to skip controller "
          "tests");
    }
  } else if (_testState == 7 && gamepads[0]->IsConnected()) {
    _testState++;
    state->AddLine("Xbox 360 controller detected");
  } else if (_testState == 7 && input->IsKeyPress('S')) {
    _testState = 12;  // skip past controller tests.
  } else if (_testState == 8) {
    _testState++;
    state->AddLine("Press the [A] button on controller 1");
  } else if (_testState == 9 && gamepads[0]->IsButtonPress(GAMEPAD_A)) {
    _testState++;
    state->SetStatus(GREEN, "[Test Passed]");
    state->AddLine("Pull the left trigger on controller 1");
  } else if (_testState == 10 && gamepads[0]->GetLeftTrigger() == 255) {
    _testState++;
    state->SetStatus(GREEN, "[Test Passed]");
    state->AddLine(
        "Pull the right trigger on controller 1, press [A] if the controller "
        "vibrates, [B] if it does not.");
  } else if (_testState == 11 && gamepads[0]->IsButtonPress(GAMEPAD_A)) {
    _testState++;
    state->SetStatus(GREEN, "[Test Passed]");
    gamepads[0]->SetVibrationSpeed(0, 0);
  } else if (_testState == 11 && gamepads[0]->IsButtonPress(GAMEPAD_B)) {
    _testState++;
    state->SetStatus(RED, "[Test Failed]");
    gamepads[0]->SetVibrationSpeed(0, 0);
  }
}

}  // namespace Test
}  // namespace MGDF