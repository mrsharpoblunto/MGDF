#pragma once

#include <MGDF/MGDF.h>
#include <dxgi1_2.h>

#include <MGDF/ComObject.hpp>
#include <optional>

namespace MGDF {
namespace core {

// Render-thread operations; startup and teardown run with that thread stopped.
class IRenderBackend {
 public:
  virtual ~IRenderBackend() = default;

  virtual ComObject<IUnknown> RTGetDevice() = 0;
  virtual ComObject<IUnknown> RTGetBackBuffer() = 0;
  virtual ComObject<IUnknown> RTGetDepthStencilBuffer() = 0;
  virtual ComObject<IDXGIAdapter> RTGetAdapter() = 0;
  virtual MGDFBackBufferInfo RTGetBackBufferInfo() const = 0;
  virtual ComObject<ID3D12CommandQueue> RTGetQueue(D3D12_COMMAND_LIST_TYPE) {
    return {};
  }
  virtual ComObject<ID3D12Fence> RTGetFrameFence() { return {}; }
  virtual MGDFFrameInfo RTGetCurrentFrame() const { return {}; }
  virtual HRESULT RTEndFrame() { return S_OK; }
  virtual bool RTInit() = 0;
  virtual bool RTIsInitialized() const = 0;
  virtual void RTUninit(bool exclusiveMode) = 0;
  virtual void RTCreateSwapChain(HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc,
                                 std::optional<UINT> maxFrameLatency) = 0;
  virtual HRESULT RTResizeBackBuffer(const DXGI_SWAP_CHAIN_DESC1 &desc) = 0;
  virtual void RTSetExclusiveFullscreen() = 0;
  virtual void RTSetWindowed() = 0;
  virtual bool RTWaitForFrame() = 0;
  virtual void RTWaitForGpuIdle() = 0;
  virtual void RTClear() = 0;
  virtual HRESULT RTPresent(UINT syncInterval, UINT flags) = 0;
  virtual HRESULT RTGetDeviceRemovedReason() const = 0;
};

}  // namespace core
}  // namespace MGDF
