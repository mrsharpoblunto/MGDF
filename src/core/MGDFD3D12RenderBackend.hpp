#pragma once

#include <dxgi1_6.h>

#include <array>
#include <functional>

#include "MGDFD3D12FrameState.hpp"
#include "core.impl/MGDFOpenXR.hpp"
#include "core.impl/MGDFStreamline.hpp"

namespace MGDF {
namespace core {

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

class D3D12RenderBackend {
 public:
  OpenXR &VR() { return _vr; }
  bool RTMatchesVRAdapter() {
    const auto adapter = RTGetAdapter();
    DXGI_ADAPTER_DESC desc{};
    if (!adapter || FAILED(adapter->GetDesc(&desc))) return false;
    const auto luid = _vr.Requirements().Adapter;
    return luid.LowPart == desc.AdapterLuid.LowPart &&
           luid.HighPart == desc.AdapterLuid.HighPart;
  }

  D3D12RenderBackend(
      const ComObject<IDXGIFactory6> &factory,
      const MGDFGraphicsRequirements &requirements,
      std::function<void(const char *, const char *)> fatalError,
      bool allowSoftwareAdapter = false,
      std::optional<std::wstring> streamlineDirectory = std::nullopt);
  bool RTPrepareVR(const char *applicationName);
  bool RTStartVR();
  bool RTInit();
  bool RTIsInitialized() const;
  void RTUninit(bool exclusiveMode);
  void RTCreateSwapChain(HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc,
                         DXGI_COLOR_SPACE_TYPE colorSpace,
                         std::optional<UINT> maxFrameLatency,
                         UINT32 generatedFrames);
  HRESULT RTResizeBackBuffer(const DXGI_SWAP_CHAIN_DESC1 &desc);
  void RTSetHDRMetadata(const std::optional<DXGI_HDR_METADATA_HDR10> &metadata);
  bool RTSetExclusiveFullscreen();
  void RTSetWindowed();
  bool RTWaitForFrame(MGDFLowLatencyMode lowLatency, bool headsetPaced = false);
  void RTWaitForGpuIdle();
  void RTClear();
  HRESULT RTPresent(UINT syncInterval, UINT flags);
  HRESULT RTEndFrame();
  HRESULT RTGetDeviceRemovedReason() const;
  ComObject<ID3D12Device10> RTGetDevice();
  ComObject<IDXGIAdapter> RTGetAdapter();
  MGDFBackBufferInfo RTGetBackBufferInfo() const;
  ComObject<ID3D12CommandQueue> RTGetQueue(D3D12_COMMAND_LIST_TYPE type);
  ComObject<ID3D12Fence> RTGetFrameFence();
  MGDFFrameInfo RTGetCurrentFrame() const;
  Streamline *GetStreamline();
  GraphicsSupport RTGetGraphicsSupport() const;

 private:
  OpenXR _vr;
  static constexpr UINT BackBufferCount = 3;
  ComObject<ID3D12Device10> RTTryCreateDevice(IDXGIAdapter *adapter);
  ComObject<ID3D12CommandQueue> RTCreateQueue(
      ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type,
      ComObject<ID3D12CommandQueue> &proxy);
  void RTReleaseBackBuffers();
  void RTReleaseSwapChain();
  void RTWaitForFence(ID3D12Fence *fence, UINT64 value);
  void FatalError(const char *sender, const char *message);
  void Check(HRESULT result, const char *message);

  const ComObject<IDXGIFactory6> &_rtFactory;
  MGDFGraphicsRequirements _requirements;
  std::function<void(const char *, const char *)> _fatalError;
  bool _allowSoftwareAdapter = false;
  ComObject<ID3D12Device10> _device;
  ComObject<IDXGIAdapter> _adapter;
  Streamline _streamline;
  bool _rayTracing = false;
  bool _shaderExecutionReordering = false;
  ComObject<ID3D12CommandQueue> _directQueue, _computeQueue, _copyQueue;
  // Streamline proxy of the direct queue; null without Streamline.
  ComObject<ID3D12CommandQueue> _proxyDirectQueue;
  ComObject<ID3D12Fence> _frameFence, _idleFence;
  UINT64 _idleValue = 0;
  HANDLE _fenceEvent = nullptr;
  HANDLE _frameWaitable = nullptr;
  // _swapChain is the Streamline proxy when Streamline is active. Use it for
  // the calls Streamline hooks (Present, ResizeBuffers, GetBuffer,
  // GetCurrentBackBufferIndex, SetFullscreenState) and the native one for the
  // rest.
  ComObject<IDXGISwapChain3> _swapChain, _nativeSwapChain;
  DXGI_SWAP_CHAIN_DESC1 _swapDesc{};
  DXGI_COLOR_SPACE_TYPE _colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
  std::optional<DXGI_HDR_METADATA_HDR10> _hdrMetadata;
  ComObject<ID3D12DescriptorHeap> _rtvHeap;
  UINT _rtvSize = 0;
  std::array<ComObject<ID3D12Resource>, BackBufferCount> _backBuffers;
  std::array<ComObject<ID3D12CommandAllocator>, D3D12FrameState::FramesInFlight>
      _clearAllocators;
  std::array<ComObject<ID3D12GraphicsCommandList>,
             D3D12FrameState::FramesInFlight>
      _clearLists;
  D3D12FrameState _frames;
  MGDFFrameInfo _frame{};
  DWORD _debugCallback = 0;
  bool _hasDebugCallback = false;
};

}  // namespace core
}  // namespace MGDF
