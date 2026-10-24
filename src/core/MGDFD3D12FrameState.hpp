#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace MGDF {
namespace core {

inline constexpr bool PreferD3D12Adapter(bool software, bool supported,
                                         uint64_t memory,
                                         std::optional<uint64_t> bestMemory) {
  return !software && supported && (!bestMemory || memory > *bestMemory);
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
