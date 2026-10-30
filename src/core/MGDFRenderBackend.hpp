#pragma once

#include <MGDF/MGDF.h>
#include <dxgi1_5.h>

#include <MGDF/ComObject.hpp>
#include <optional>

#include "core.impl/MGDFStreamline.hpp"

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
  // Null for backends without Streamline. Any thread may use the object.
  virtual Streamline *GetStreamline() { return nullptr; }
  virtual GraphicsSupport RTGetGraphicsSupport() const { return {}; }
  virtual bool RTInit() = 0;
  virtual bool RTIsInitialized() const = 0;
  virtual void RTUninit(bool exclusiveMode) = 0;
  virtual void RTCreateSwapChain(HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc,
                                 DXGI_COLOR_SPACE_TYPE colorSpace,
                                 std::optional<UINT> maxFrameLatency,
                                 UINT32 generatedFrames) = 0;
  virtual HRESULT RTResizeBackBuffer(const DXGI_SWAP_CHAIN_DESC1 &desc) = 0;
  // Kept for later swap chains; applied now to an HDR10 swap chain.
  virtual void RTSetHDRMetadata(
      const std::optional<DXGI_HDR_METADATA_HDR10> &metadata) = 0;
  virtual bool RTSetExclusiveFullscreen() = 0;
  virtual void RTSetWindowed() = 0;
  virtual bool RTWaitForFrame(MGDFLowLatencyMode lowLatency) = 0;
  virtual void RTWaitForGpuIdle() = 0;
  virtual void RTClear() = 0;
  virtual HRESULT RTPresent(UINT syncInterval, UINT flags) = 0;
  virtual HRESULT RTGetDeviceRemovedReason() const = 0;
};

// ResizeBuffers does not keep the color space, so call this after each create
// and resize. Use the native swap chain: Streamline does not hook these calls.
inline bool SetSwapChainColorSpace(IDXGISwapChain3 *swapChain,
                                   DXGI_COLOR_SPACE_TYPE colorSpace) {
  UINT support = 0;
  return SUCCEEDED(swapChain->CheckColorSpaceSupport(colorSpace, &support)) &&
         (support & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) &&
         SUCCEEDED(swapChain->SetColorSpace1(colorSpace));
}

// HDR10 metadata describes PQ content, so other color spaces get none
// (S_FALSE). Use the native swap chain.
inline HRESULT SetSwapChainHDRMetadata(
    IDXGISwapChain3 *swapChain, DXGI_COLOR_SPACE_TYPE colorSpace,
    std::optional<DXGI_HDR_METADATA_HDR10> metadata) {
  if (colorSpace != DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) return S_FALSE;
  ComObject<IDXGISwapChain4> chain;
  if (FAILED(swapChain->QueryInterface(IID_PPV_ARGS(chain.Assign()))))
    return E_NOINTERFACE;
  return metadata
             ? chain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10,
                                     sizeof(*metadata), &*metadata)
             : chain->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_NONE, 0, nullptr);
}

}  // namespace core
}  // namespace MGDF
