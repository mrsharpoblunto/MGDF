#pragma once

#include <MGDF/MGDF.h>

#include <array>
#include <cmath>
#include <mutex>
#include <optional>
#include <utility>

namespace MGDF::core {

struct VRInputSnapshot {
  std::array<MGDFVRController, 2> Controllers{};
  MGDFVRPose Gaze{};
  HRESULT ControllersStatus = E_ILLEGAL_METHOD_CALL;
  HRESULT GazeStatus = E_ILLEGAL_METHOD_CALL;
};

// Shared by the render and sim threads; contains no runtime handles.
class VRInputState {
 public:
  struct Haptic {
    float Amplitude;
    float Seconds;
    bool Stop;
  };
  void Publish(const VRInputSnapshot &snapshot) {
    std::lock_guard lock(_mutex);
    _snapshot = snapshot;
    for (size_t i = 0; i < _haptics.size(); ++i)
      if (!_snapshot.Controllers[i].Connected) _haptics[i].reset();
  }
  BOOL GetController(MGDFVRHand hand, MGDFVRController *controller) const {
    if (!controller) return FALSE;
    *controller = {};
    if (hand != MGDF_VR_HAND_LEFT && hand != MGDF_VR_HAND_RIGHT) return FALSE;
    std::lock_guard lock(_mutex);
    *controller = _snapshot.Controllers[hand];
    return controller->Connected;
  }
  BOOL GetGaze(MGDFVRPose *pose) const {
    if (!pose) return FALSE;
    *pose = {};
    std::lock_guard lock(_mutex);
    if (_snapshot.GazeStatus != S_OK) return FALSE;
    *pose = _snapshot.Gaze;
    return TRUE;
  }
  BOOL QueueHaptic(MGDFVRHand hand, float amplitude, float seconds, bool stop) {
    if ((hand != MGDF_VR_HAND_LEFT && hand != MGDF_VR_HAND_RIGHT) ||
        !std::isfinite(amplitude) || amplitude < 0 || amplitude > 1 ||
        !std::isfinite(seconds) || (!stop && (seconds <= 0 || seconds > 60)))
      return FALSE;
    std::lock_guard lock(_mutex);
    if (!_snapshot.Controllers[hand].Connected) return FALSE;
    _haptics[hand] = {amplitude, seconds, stop};
    return TRUE;
  }
  std::array<std::optional<Haptic>, 2> TakeHaptics() {
    std::lock_guard lock(_mutex);
    return std::exchange(_haptics, {});
  }

 private:
  mutable std::mutex _mutex;
  VRInputSnapshot _snapshot;
  std::array<std::optional<Haptic>, 2> _haptics;
};

}  // namespace MGDF::core
