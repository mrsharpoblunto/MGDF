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
                         std::optional<UINT> maxFrameLatency) final;
  HRESULT RTResizeBackBuffer(const DXGI_SWAP_CHAIN_DESC1 &desc) final;
  void RTSetExclusiveFullscreen() final;
  void RTSetWindowed() final;
  bool RTWaitForFrame() final;
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

 private:
  static constexpr UINT BackBufferCount = 3;
  ComObject<ID3D12Device10> RTTryCreateDevice(IDXGIAdapter *adapter);
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
  ComObject<ID3D12CommandQueue> _directQueue, _computeQueue, _copyQueue;
  ComObject<ID3D12Fence> _frameFence, _idleFence;
  UINT64 _idleValue = 0;
  HANDLE _fenceEvent = nullptr;
  HANDLE _frameWaitable = nullptr;
  ComObject<IDXGISwapChain3> _swapChain;
  DXGI_SWAP_CHAIN_DESC1 _swapDesc{};
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
