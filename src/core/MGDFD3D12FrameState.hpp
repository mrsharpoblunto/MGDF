#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <optional>

namespace MGDF {
namespace core {

// Ray tracing support ranks before dedicated memory.
struct D3D12AdapterScore {
  bool RayTracing = false;
  uint64_t Memory = 0;
  auto operator<=>(const D3D12AdapterScore &) const = default;
};

inline constexpr bool PreferD3D12Adapter(
    bool software, bool supported, D3D12AdapterScore score,
    std::optional<D3D12AdapterScore> best) {
  return !software && supported && (!best || score > *best);
}

class D3D12FrameState {
 public:
  static constexpr uint32_t FramesInFlight = 2;
  uint64_t Ordinal() const { return _ordinal; }
  uint32_t Slot() const {
    return static_cast<uint32_t>(_ordinal % FramesInFlight);
  }
  uint64_t PreviousFence() const { return _slots[Slot()]; }
  void Submitted() {
    _slots[Slot()] = _ordinal;
    ++_ordinal;
  }

 private:
  uint64_t _ordinal = 1;
  std::array<uint64_t, FramesInFlight> _slots{};
};

}  // namespace core
}  // namespace MGDF
