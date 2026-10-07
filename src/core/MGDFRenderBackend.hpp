#pragma once

#include <dxgi1_2.h>

#include <optional>

namespace MGDF {
namespace core {

// Render-thread operations; startup and teardown run with that thread stopped.
class IRenderBackend {
 public:
  virtual ~IRenderBackend() = default;

  virtual bool RTInit() = 0;
  virtual bool RTIsInitialized() const = 0;
  virtual void RTUninit(bool exclusiveMode) = 0;
  virtual void RTCreateSwapChain(HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc,
                                 std::optional<UINT> maxFrameLatency) = 0;
  virtual HRESULT RTResizeBackBuffer(const DXGI_SWAP_CHAIN_DESC1 &desc) = 0;
  virtual void RTSetExclusiveFullscreen() = 0;
  virtual void RTSetWindowed() = 0;
  virtual void RTWaitForFrame() = 0;
  virtual void RTWaitForGpuIdle() = 0;
  virtual void RTClear() = 0;
  virtual HRESULT RTPresent(UINT syncInterval, UINT flags) = 0;
  virtual HRESULT RTGetDeviceRemovedReason() const = 0;
};

}  // namespace core
}  // namespace MGDF
