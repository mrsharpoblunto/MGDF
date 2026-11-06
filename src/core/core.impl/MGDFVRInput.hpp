#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <mutex>
#include <utility>

namespace MGDF::core {

struct VRControllerState {
  BOOL Connected = FALSE;
  MGDFVRControllerProfile Profile = MGDF_VR_PROFILE_NONE;
  MGDFVRPose Grip{};
  MGDFVRPose Aim{};
  FLOAT Trigger = 0;
  FLOAT Squeeze = 0;
  FLOAT StickX = 0;
  FLOAT StickY = 0;
  UINT32 Buttons = 0;
  UINT32 Touches = 0;
};
struct VRInputSnapshot {
  std::array<VRControllerState, 2> Controllers{};
  MGDFVRPose Gaze{};
  HRESULT ControllersStatus = E_ILLEGAL_METHOD_CALL;
  HRESULT GazeStatus = E_ILLEGAL_METHOD_CALL;
};

// Shared by the render and sim threads; contains no runtime handles.
class VRInputState {
 public:
  void Publish(const VRInputSnapshot &snapshot) {
    std::lock_guard lock(_mutex);
    for (size_t i = 0; i < _pendingPress.size(); ++i) {
      if (snapshot.Controllers[i].Connected) {
        // Capture down-to-up transitions even when no sim update reads down.
        _pendingPress[i] |=
            _snapshot.Controllers[i].Buttons & ~snapshot.Controllers[i].Buttons;
      } else {
        _pendingPress[i] = 0;
      }
    }
    _snapshot = snapshot;
  }
  void ProcessSim() {
    std::lock_guard lock(_mutex);
    _simSnapshot = _snapshot;
    _press = std::exchange(_pendingPress, {});
  }
  void ClearPresses() {
    std::lock_guard lock(_mutex);
    _pendingPress = _press = {};
  }
  VRControllerState Controller(MGDFVRHand hand) const {
    std::lock_guard lock(_mutex);
    return _simSnapshot.Controllers[hand];
  }
  BOOL TakePress(MGDFVRHand hand, MGDFVRButton button) {
    std::lock_guard lock(_mutex);
    const auto bits = static_cast<UINT32>(button);
    if (!bits || (_press[hand] & bits) != bits) return FALSE;
    _press[hand] &= ~bits;
    return TRUE;
  }
  BOOL GetGaze(MGDFVRPose *pose) const {
    if (!pose) return FALSE;
    *pose = {};
    std::lock_guard lock(_mutex);
    if (_simSnapshot.GazeStatus != S_OK) return FALSE;
    *pose = _simSnapshot.Gaze;
    return TRUE;
  }
  void SetVibration(MGDFVRHand hand, float amplitude) {
    std::lock_guard lock(_mutex);
    _vibration[hand] =
        std::isnan(amplitude) ? 0 : std::clamp(amplitude, 0.0f, 1.0f);
  }
  std::array<float, 2> Vibration() const {
    std::lock_guard lock(_mutex);
    return _vibration;
  }

 private:
  mutable std::mutex _mutex;
  VRInputSnapshot _snapshot;
  VRInputSnapshot _simSnapshot;
  std::array<UINT32, 2> _pendingPress{};
  std::array<UINT32, 2> _press{};
  std::array<float, 2> _vibration{};
};

class VRController final : public ComBase<IMGDFVRController> {
 public:
  VRController(std::shared_ptr<VRInputState> input, MGDFVRHand hand)
      : _input(std::move(input)), _hand(hand) {
    _ASSERTE(hand == MGDF_VR_HAND_LEFT || hand == MGDF_VR_HAND_RIGHT);
  }
  MGDFVRHand __stdcall GetHand() final { return _hand; }
  BOOL __stdcall IsConnected() final { return State().Connected; }
  MGDFVRControllerProfile __stdcall GetProfile() final {
    return State().Profile;
  }
  BOOL __stdcall IsButtonDown(MGDFVRButton button) final {
    return button && (State().Buttons & button) == static_cast<UINT32>(button);
  }
  BOOL __stdcall IsButtonUp(MGDFVRButton button) final {
    return !IsButtonDown(button);
  }
  BOOL __stdcall IsButtonPress(MGDFVRButton button) final {
    return _input->TakePress(_hand, button);
  }
  BOOL __stdcall IsTouched(MGDFVRTouch touch) final {
    return touch && (State().Touches & touch) == static_cast<UINT32>(touch);
  }
  FLOAT __stdcall GetTrigger() final { return State().Trigger; }
  FLOAT __stdcall GetSqueeze() final { return State().Squeeze; }
  FLOAT __stdcall GetThumbX() final { return State().StickX; }
  FLOAT __stdcall GetThumbY() final { return State().StickY; }
  void __stdcall GetGripPose(MGDFVRPose *pose) final {
    if (pose) *pose = State().Grip;
  }
  void __stdcall GetAimPose(MGDFVRPose *pose) final {
    if (pose) *pose = State().Aim;
  }
  void __stdcall SetVibration(FLOAT amplitude) final {
    _input->SetVibration(_hand, amplitude);
  }

 private:
  VRControllerState State() const { return _input->Controller(_hand); }
  const std::shared_ptr<VRInputState> _input;
  const MGDFVRHand _hand;
};

}  // namespace MGDF::core
