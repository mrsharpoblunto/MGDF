#pragma once

#include <dxgi1_6.h>

#include <array>
#include <functional>

#include "MGDFD3D12FrameState.hpp"
#include "MGDFRenderBackend.hpp"

namespace MGDF {
namespace core {

class D3D12RenderBackend : public IRenderBackend {
 public:
  D3D12RenderBackend(
      const ComObject<IDXGIFactory6> &factory,
      const MGDFGraphicsRequirements &requirements,
      std::function<void(const char *, const char *)> fatalError);
  bool RTInit() final;
  bool RTIsInitialized() const final;
  void RTUninit(bool exclusiveMode) final;
  void RTCreateSwapChain(HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc,
                         DXGI_COLOR_SPACE_TYPE colorSpace,
                         std::optional<UINT> maxFrameLatency,
                         UINT32 generatedFrames) final;
  HRESULT RTResizeBackBuffer(const DXGI_SWAP_CHAIN_DESC1 &desc) final;
  void RTSetHDRMetadata(
      const std::optional<DXGI_HDR_METADATA_HDR10> &metadata) final;
  bool RTSetExclusiveFullscreen() final;
  void RTSetWindowed() final;
  bool RTWaitForFrame(MGDFLowLatencyMode lowLatency) final;
  void RTWaitForGpuIdle() final;
  void RTClear() final;
  HRESULT RTPresent(UINT syncInterval, UINT flags) final;
  HRESULT RTEndFrame() final;
  HRESULT RTGetDeviceRemovedReason() const final;
  ComObject<IUnknown> RTGetDevice() final;
  ComObject<IUnknown> RTGetBackBuffer() final;
  ComObject<IUnknown> RTGetDepthStencilBuffer() final;
  ComObject<IDXGIAdapter> RTGetAdapter() final;
  MGDFBackBufferInfo RTGetBackBufferInfo() const final;
  ComObject<ID3D12CommandQueue> RTGetQueue(D3D12_COMMAND_LIST_TYPE type) final;
  ComObject<ID3D12Fence> RTGetFrameFence() final;
  MGDFFrameInfo RTGetCurrentFrame() const final;
  Streamline *GetStreamline() final;

 private:
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
  ComObject<ID3D12Device10> _device;
  ComObject<IDXGIAdapter> _adapter;
  Streamline _streamline;
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
