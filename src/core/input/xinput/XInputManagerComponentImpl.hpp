#pragma once

#include <MGDF/MGDF.h>

#include <atomic>
#include <bitset>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include "../MGDFInputManagerComponent.hpp"
#include "XInputGamepad.hpp"

namespace MGDF {
namespace core {
namespace input {
namespace xinput {

/**
 reference implementation of the inputmanager interface
 \author gcconner
*/
class XInputManagerComponent : public IInputManagerComponent {
 public:
  XInputManagerComponent();
  virtual ~XInputManagerComponent(void);

  void HandleInput(std::function<RAWINPUT *()> getInput) final;
  void HandleInput(INT32 mouseX, INT32 mouseY) final;
  void ClearInput() final;
  void ProcessSim() final;
  bool GetShowCursor() final;
  MGDFCursor GetCursorShape() final;

  void __stdcall ShowCursor(BOOL show) final;
  void __stdcall SetCursorShape(MGDFCursor cursor) final;
  BOOL __stdcall IsKeyDown(UINT16 key) final;
  BOOL __stdcall IsKeyUp(UINT16 key) final;
  BOOL __stdcall IsKeyPress(UINT16 key) final;
  INT32 __stdcall GetMouseX(void) final;
  INT32 __stdcall GetMouseY(void) final;
  INT32 __stdcall GetMouseDX(void) final;
  INT32 __stdcall GetMouseDY(void) final;
  INT16 __stdcall GetMouseDZ(void) final;
  BOOL __stdcall IsButtonDown(MGDFMouse mouseButton) final;
  BOOL __stdcall IsButtonUp(MGDFMouse mouseButton) final;
  BOOL __stdcall IsButtonClicked(MGDFMouse mouseButton) final;
  UINT64 GetGamepadCount() final;
  void __stdcall GetGamepads(IMGDFGamepad **gamepads) final;

 private:
  std::mutex _simMutex;

  // pending keyboard state (input thread)
  bool _pendingKeyDown[256]{};
  std::bitset<256> _changedKeys;
  std::bitset<256> _pendingKeyPress;
  bool _resetPending = false;

  std::atomic_bool _showCursor;
  std::atomic<MGDFCursor> _cursor;

  // current keyboard state (sim thread)
  bool _keyDown[256]{};
  bool _keyPress[256]{};

  // pending mouse state (input thread)
  INT32 _pendingMouseDX, _pendingMouseDY;
  INT16 _pendingMouseDZ;
  UINT8 _pendingMouseButtonDown[3]{};
  bool _pendingMouseButtonClick[3]{};
  INT32 _pendingMouseX, _pendingMouseY;

  // current mouse state (sim thread)
  INT32 _mouseDX, _mouseDY;
  INT16 _mouseDZ;
  bool _mouseButtonDown[3]{};
  bool _mouseButtonClick[3]{};
  INT32 _mouseX = 0, _mouseY = 0;

  std::vector<ComObject<XInputGamepad>> _gamepads;
  std::set<XInputGamepad *> _connectedGamepads;
  std::set<XInputGamepad *> _disconnectedGamepads;
  std::thread _gamepadCheckThread;
  std::atomic_bool _checkThreadRunning{false};
  std::mutex _gamepadMutex;
};

}  // namespace xinput
}  // namespace input
}  // namespace core
}  // namespace MGDF