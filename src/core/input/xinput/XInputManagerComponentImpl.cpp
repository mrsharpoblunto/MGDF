#include "stdafx.h"

#include "XInputManagerComponentImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

XInputManagerComponent::XInputManagerComponent()
    : _showCursor(true),
      _pendingMouseX(0),
      _pendingMouseY(0),
      _pendingMouseDX(0L),
      _pendingMouseDY(0L),
      _pendingMouseDZ(0),
      _mouseDX(0L),
      _mouseDY(0L),
      _mouseDZ(0),
      _pendingKeyDownEventsLength(0),
      _pendingKeyPressEventsLength(0) {
  for (INT32 i = 0; i < XUSER_MAX_COUNT; ++i) {
    _gamepads.push_back(ComObject(new XInputGamepad(i, true)));
    _connectedGamepads.insert(_gamepads.back());
  }

  _checkThreadRunning = true;
  _gamepadCheckThread = std::thread([this]() {
    std::unique_lock<std::mutex> lock(_gamepadMutex, std::defer_lock);
    while (_checkThreadRunning) {
      lock.lock();
      auto disconnected = _disconnectedGamepads;
      lock.unlock();

      XINPUT_STATE state = {};
      for (auto gamepad : disconnected) {
        SecureZeroMemory(&state, sizeof(XINPUT_STATE));
        const DWORD result = XInputGetState(gamepad->GetID(), &state);
        // this controller is connected again so add it back
        // to the active list
        if (result == ERROR_SUCCESS) {
          lock.lock();
          gamepad->SetIsConnected(true);
          _disconnectedGamepads.erase(gamepad);
          _connectedGamepads.insert(gamepad);
          lock.unlock();
        }
      }
      Sleep(500);
    }
  });
  ClearInput();
}

XInputManagerComponent::~XInputManagerComponent(void) {
  _checkThreadRunning = false;
  _gamepadCheckThread.join();
}

void XInputManagerComponent::ClearInput() {
  ZeroMemory(_pendingMouseButtonDown, sizeof(_pendingMouseButtonDown));
  ZeroMemory(_pendingMouseButtonClick, sizeof(_pendingMouseButtonClick));
  ZeroMemory(_mouseButtonDown, sizeof(_mouseButtonDown));
  ZeroMemory(_mouseButtonClick, sizeof(_mouseButtonClick));

  ZeroMemory(_pendingKeyDown, sizeof(_pendingKeyDown));
  ZeroMemory(_pendingKeyDownEvents, sizeof(_pendingKeyDownEvents));
  ZeroMemory(_pendingKeyPressEvents, sizeof(_pendingKeyPressEvents));
  ZeroMemory(_keyDown, sizeof(_keyDown));
  ZeroMemory(_keyPress, sizeof(_keyPress));
}

void XInputManagerComponent::HandleInput(INT32 mouseX, INT32 mouseY) {
  std::lock_guard<std::mutex> lock(_simMutex);
  _pendingMouseX = mouseX;
  _pendingMouseY = mouseY;
}

void XInputManagerComponent::HandleInput(std::function<RAWINPUT *()> getInput) {
  std::lock_guard<std::mutex> lock(_simMutex);

  while (true) {
    RAWINPUT *input = getInput();
    if (!input) {
      break;
    }
    if (input->header.dwType == RIM_TYPEKEYBOARD) {
      // we only know how to deal with keys
      if (input->data.keyboard.VKey > UINT8_MAX) return;

      UINT8 key = static_cast<UINT8>(input->data.keyboard.VKey);
      if ((input->data.keyboard.Flags & RI_KEY_MAKE) == RI_KEY_MAKE &&
          _pendingKeyDown[key] != 1) {
        _pendingKeyDown[key] = 1;
        _pendingKeyDownEvents[_pendingKeyDownEventsLength] = key;
        _pendingKeyDownEventsLength++;
      }
      if ((input->data.keyboard.Flags & RI_KEY_BREAK) == RI_KEY_BREAK &&
          _pendingKeyDown[key] != 2) {
        _pendingKeyDown[key] = 2;
        _pendingKeyDownEvents[_pendingKeyDownEventsLength] = key;
        _pendingKeyDownEventsLength++;

        _pendingKeyPressEvents[_pendingKeyPressEventsLength] = key;
        _pendingKeyPressEventsLength++;
      }
    } else if (input->header.dwType == RIM_TYPEMOUSE) {
      if ((input->data.mouse.usFlags & MOUSE_MOVE_RELATIVE) ==
          MOUSE_MOVE_RELATIVE) {
        _pendingMouseDX += input->data.mouse.lLastX;
        _pendingMouseDY += input->data.mouse.lLastY;
      }

      // mouse scrollwheel
      if ((input->data.mouse.usButtonFlags & RI_MOUSE_WHEEL) ==
          RI_MOUSE_WHEEL) {
        _pendingMouseDZ += static_cast<INT16>(input->data.mouse.usButtonData);
      }
      // mouse button states
      if ((input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) ==
          RI_MOUSE_LEFT_BUTTON_DOWN) {
        _pendingMouseButtonDown[MGDF_MOUSE_LEFT] = 1;
      }
      if ((input->data.mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) ==
          RI_MOUSE_LEFT_BUTTON_UP) {
        _pendingMouseButtonDown[MGDF_MOUSE_LEFT] = 2;
        _pendingMouseButtonClick[MGDF_MOUSE_LEFT] = true;
      }
      if ((input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) ==
          RI_MOUSE_MIDDLE_BUTTON_DOWN) {
        _pendingMouseButtonDown[MGDF_MOUSE_MIDDLE] = 1;
      }
      if ((input->data.mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) ==
          RI_MOUSE_MIDDLE_BUTTON_UP) {
        _pendingMouseButtonDown[MGDF_MOUSE_MIDDLE] = 2;
        _pendingMouseButtonClick[MGDF_MOUSE_MIDDLE] = true;
      }
      if ((input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) ==
          RI_MOUSE_RIGHT_BUTTON_DOWN) {
        _pendingMouseButtonDown[MGDF_MOUSE_RIGHT] = 1;
      }
      if ((input->data.mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) ==
          RI_MOUSE_RIGHT_BUTTON_UP) {
        _pendingMouseButtonDown[MGDF_MOUSE_RIGHT] = 2;
        _pendingMouseButtonClick[MGDF_MOUSE_RIGHT] = true;
      }
    }
  }
}

void XInputManagerComponent::ProcessSim() {
  // handleinput occurs on a different thread to processinput so
  // we need to sync any access to the pending input state
  {
    std::lock_guard<std::mutex> lock(_simMutex);
    // update keyboard and mouse state

    // mouse position
    _mouseX = _pendingMouseX;
    _mouseY = _pendingMouseY;

    // mouse movement
    _mouseDX = _pendingMouseDX;
    _mouseDY = _pendingMouseDY;
    _mouseDZ = _pendingMouseDZ;
    _pendingMouseDX = 0L;
    _pendingMouseDY = 0L;
    _pendingMouseDZ = 0;

    // mouse button states
    if (_pendingMouseButtonDown[MGDF_MOUSE_LEFT]) {
      _mouseButtonDown[MGDF_MOUSE_LEFT] =
          _pendingMouseButtonDown[MGDF_MOUSE_LEFT] == 1;
    }
    if (_pendingMouseButtonDown[MGDF_MOUSE_MIDDLE]) {
      _mouseButtonDown[MGDF_MOUSE_MIDDLE] =
          _pendingMouseButtonDown[MGDF_MOUSE_MIDDLE] == 1;
    }
    if (_pendingMouseButtonDown[MGDF_MOUSE_RIGHT]) {
      _mouseButtonDown[MGDF_MOUSE_RIGHT] =
          _pendingMouseButtonDown[MGDF_MOUSE_RIGHT] == 1;
    }
    ZeroMemory(_pendingMouseButtonDown, sizeof(_pendingMouseButtonDown));

    // mouse clicks
    _mouseButtonClick[MGDF_MOUSE_LEFT] =
        _pendingMouseButtonClick[MGDF_MOUSE_LEFT];
    _mouseButtonClick[MGDF_MOUSE_MIDDLE] =
        _pendingMouseButtonClick[MGDF_MOUSE_MIDDLE];
    _mouseButtonClick[MGDF_MOUSE_RIGHT] =
        _pendingMouseButtonClick[MGDF_MOUSE_RIGHT];
    _pendingMouseButtonClick[MGDF_MOUSE_LEFT] = false;
    _pendingMouseButtonClick[MGDF_MOUSE_MIDDLE] = false;
    _pendingMouseButtonClick[MGDF_MOUSE_RIGHT] = false;
    ZeroMemory(_pendingMouseButtonClick, sizeof(_pendingMouseButtonClick));

    // keyboard events
    for (UINT8 i = 0; i < _pendingKeyDownEventsLength; ++i) {
      UINT8 key = _pendingKeyDownEvents[i];
      _keyDown[key] = _pendingKeyDown[key] == 1;
    }
    ZeroMemory(_pendingKeyDown, sizeof(_pendingKeyDown));
    _pendingKeyDownEventsLength = 0;

    ZeroMemory(_keyPress, sizeof(_keyPress));
    for (UINT8 i = 0; i < _pendingKeyPressEventsLength; ++i) {
      UINT8 key = _pendingKeyPressEvents[i];
      _keyPress[key] = true;
    }
    _pendingKeyPressEventsLength = 0;
  }

  // read controller states
  {
    std::unique_lock<std::mutex> lock(_gamepadMutex);
    auto connected = _connectedGamepads;
    lock.unlock();
    XINPUT_STATE state = {};
    for (auto gamepad : connected) {
      SecureZeroMemory(&state, sizeof(XINPUT_STATE));
      const DWORD result = XInputGetState(gamepad->GetID(), &state);
      if (result == ERROR_SUCCESS) {
        gamepad->UpdateState(state);
      } else {
        lock.lock();
        gamepad->SetIsConnected(false);
        _disconnectedGamepads.insert(gamepad);
        _connectedGamepads.erase(gamepad);
        lock.unlock();
      }
    }
  }
}

bool XInputManagerComponent::GetShowCursor() { return _showCursor.load(); }

void XInputManagerComponent::ShowCursor(BOOL show) { _showCursor.store(show); }

BOOL XInputManagerComponent::IsKeyDown(UINT16 key) { return _keyDown[key]; }

BOOL XInputManagerComponent::IsKeyUp(UINT16 key) { return !_keyDown[key]; }

BOOL XInputManagerComponent::IsKeyPress(UINT16 key) { return _keyPress[key]; }

INT32 XInputManagerComponent::GetMouseX(void) { return _mouseX; }

INT32 XInputManagerComponent::GetMouseY(void) { return _mouseY; }

INT32 XInputManagerComponent::GetMouseDX() { return _mouseDX; }

INT32 XInputManagerComponent::GetMouseDY() { return _mouseDY; }

INT16 XInputManagerComponent::GetMouseDZ() { return _mouseDZ; }

BOOL XInputManagerComponent::IsButtonDown(MGDFMouse mouseButton) {
  return _mouseButtonDown[mouseButton];
}

BOOL XInputManagerComponent::IsButtonUp(MGDFMouse mouseButton) {
  return !_mouseButtonDown[mouseButton];
}

BOOL XInputManagerComponent::IsButtonClicked(MGDFMouse mouseButton) {
  return _mouseButtonClick[mouseButton];
}

UINT64 XInputManagerComponent::GetGamepadCount() { return _gamepads.size(); }

void XInputManagerComponent::GetGamepads(IMGDFGamepad **gamepads) {
  IMGDFGamepad **gamepadPtr = gamepads;
  for (auto &gamepad : _gamepads) {
    gamepad.AddRawRef(gamepadPtr++);
  }
}

}  // namespace xinput
}  // namespace input
}  // namespace core
}  // namespace MGDF
