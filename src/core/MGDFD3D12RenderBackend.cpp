#include "StdAfx.h"

#include "MGDFD3D12RenderBackend.hpp"

#include <d3d12sdklayers.h>

#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"

namespace MGDF {
namespace core {

D3D12RenderBackend::D3D12RenderBackend(
    const ComObject<IDXGIFactory6> &factory,
    const MGDFGraphicsRequirements &requirements,
    std::function<void(const char *, const char *)> fatalError)
    : _rtFactory(factory),
      _requirements(requirements),
      _fatalError(std::move(fatalError)),
      _streamline(requirements.Features, Resources::Instance().RootDir()) {}

void D3D12RenderBackend::FatalError(const char *sender, const char *message) {
  _fatalError(sender, message);
}

void D3D12RenderBackend::Check(HRESULT result, const char *message) {
  if (FAILED(result)) {
    RTGetDeviceRemovedReason();
    FATALERROR(this, message << " (HRESULT " << std::hex << result << ")");
  }
}

ComObject<ID3D12Device10> D3D12RenderBackend::RTTryCreateDevice(
    IDXGIAdapter *adapter) {
  ComObject<ID3D12Device10> device;
  if (FAILED(D3D12CreateDevice(
          adapter, max(_requirements.MinFeatureLevel, D3D_FEATURE_LEVEL_11_0),
          IID_PPV_ARGS(device.Assign()))))
    return {};
  if (_requirements.MinShaderModel) {
    D3D12_FEATURE_DATA_SHADER_MODEL model{
        static_cast<D3D_SHADER_MODEL>(_requirements.MinShaderModel)};
    if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &model,
                                           sizeof(model))) ||
        static_cast<UINT32>(model.HighestShaderModel) <
            _requirements.MinShaderModel)
      return {};
  }
  return device;
}

ComObject<ID3D12CommandQueue> D3D12RenderBackend::RTCreateQueue(
    ID3D12Device *device, D3D12_COMMAND_LIST_TYPE type,
    ComObject<ID3D12CommandQueue> &proxy) {
  const D3D12_COMMAND_QUEUE_DESC desc{.Type = type};
  ComObject<ID3D12CommandQueue> queue;
  Check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(queue.Assign())),
        "Failed to create command queue");
  if (device == _device.Get()) return queue;
  // Modules get the native queue; only swap chain creation needs the proxy.
  auto native = _streamline.RTGetNative<ID3D12CommandQueue>(queue);
  if (!native) FATALERROR(this, "Failed to get native command queue");
  proxy = queue;
  return native;
}

bool D3D12RenderBackend::RTInit() {
#if defined(_DEBUG)
  ComObject<ID3D12Debug> debug;
  Check(D3D12GetDebugInterface(IID_PPV_ARGS(debug.Assign())),
        "Failed to enable D3D12 debug layer");
  debug->EnableDebugLayer();
  ComObject<ID3D12DeviceRemovedExtendedDataSettings1> dred;
  Check(D3D12GetDebugInterface(IID_PPV_ARGS(dred.Assign())),
        "Failed to enable DRED");
  dred->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  dred->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
#endif
  if (!_rtFactory) {
    _device = RTTryCreateDevice(nullptr);
    if (_device) {
      ComObject<IDXGIFactory6> factory;
      Check(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.Assign())),
            "Failed to create adapter factory");
      Check(factory->EnumAdapterByLuid(_device->GetAdapterLuid(),
                                       IID_PPV_ARGS(_adapter.Assign())),
            "Failed to find default adapter");
    }
  } else {
    std::optional<uint64_t> bestMemory;
    for (UINT i = 0;; ++i) {
      ComObject<IDXGIAdapter1> adapter;
      const auto result = _rtFactory->EnumAdapters1(i, adapter.Assign());
      if (result == DXGI_ERROR_NOT_FOUND) break;
      Check(result, "Failed to enumerate adapters");
      DXGI_ADAPTER_DESC1 desc{};
      Check(adapter->GetDesc1(&desc), "Failed to describe adapter");
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
      auto device = RTTryCreateDevice(adapter);
      if (PreferD3D12Adapter(false, static_cast<bool>(device),
                             desc.DedicatedVideoMemory, bestMemory)) {
        _device = device;
        _adapter = adapter;
        bestMemory = desc.DedicatedVideoMemory;
      }
    }
  }
  if (!_device) {
    LOG("No adapter supports the requested D3D12 feature level and shader "
        "model",
        MGDF_LOG_ERROR);
    return false;
  }
  DXGI_ADAPTER_DESC desc{};
  _adapter->GetDesc(&desc);
  char name[256]{};
  size_t converted = 0;
  wcstombs_s(&converted, name, desc.Description, _TRUNCATE);
  LOG("D3D12 adapter: " << name << ", dedicated memory "
                        << desc.DedicatedVideoMemory,
      MGDF_LOG_LOW);
#if defined(_DEBUG)
  auto info = _device.As<ID3D12InfoQueue1>();
  if (info) {
    // Frame generation back buffers belong to Streamline and have no
    // optimized clear value, so every back buffer clear would warn.
    D3D12_MESSAGE_ID deny[] = {
        D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE};
    D3D12_INFO_QUEUE_FILTER filter{
        .DenyList = {.NumIDs = _countof(deny), .pIDList = deny}};
    Check(info->AddStorageFilterEntries(&filter),
          "Failed to filter debug messages");
    Check(info->RegisterMessageCallback(
              [](D3D12_MESSAGE_CATEGORY, D3D12_MESSAGE_SEVERITY severity,
                 D3D12_MESSAGE_ID, LPCSTR description, void *) {
                if (severity <= D3D12_MESSAGE_SEVERITY_WARNING) {
                  LOG("D3D12 debug layer: " << description,
                      severity <= D3D12_MESSAGE_SEVERITY_ERROR ? MGDF_LOG_ERROR
                                                               : MGDF_LOG_LOW);
                }
              },
              D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &_debugCallback),
          "Failed to register debug callback");
    _hasDebugCallback = true;
  }
#endif
  _streamline.RTInit(_device, _adapter.As<IDXGIAdapter1>());
  // Streamline must see every queue, so they come from its proxy device.
  const auto proxyDevice = _streamline.RTGetProxyDevice();
  ID3D12Device *queueDevice = proxyDevice ? proxyDevice.Get() : _device.Get();
  ComObject<ID3D12CommandQueue> unusedProxy;
  _directQueue = RTCreateQueue(queueDevice, D3D12_COMMAND_LIST_TYPE_DIRECT,
                               _proxyDirectQueue);
  _computeQueue =
      RTCreateQueue(queueDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE, unusedProxy);
  _copyQueue =
      RTCreateQueue(queueDevice, D3D12_COMMAND_LIST_TYPE_COPY, unusedProxy);
  Check(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                             IID_PPV_ARGS(_frameFence.Assign())),
        "Failed to create frame fence");
  // Idle signals must not advance the module-visible frame ordinal.
  Check(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
                             IID_PPV_ARGS(_idleFence.Assign())),
        "Failed to create idle fence");
  _frames = {};
  _idleValue = 0;
  _fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
  if (!_fenceEvent) FATALERROR(this, "Failed to create fence event");
  const D3D12_DESCRIPTOR_HEAP_DESC heap{.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                        .NumDescriptors = BackBufferCount};
  Check(_device->CreateDescriptorHeap(&heap, IID_PPV_ARGS(_rtvHeap.Assign())),
        "Failed to create RTV heap");
  _rtvSize =
      _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  for (UINT i = 0; i < D3D12FrameState::FramesInFlight; ++i) {
    Check(_device->CreateCommandAllocator(
              D3D12_COMMAND_LIST_TYPE_DIRECT,
              IID_PPV_ARGS(_clearAllocators[i].Assign())),
          "Failed to create clear allocator");
    Check(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                                     _clearAllocators[i], nullptr,
                                     IID_PPV_ARGS(_clearLists[i].Assign())),
          "Failed to create clear list");
    Check(_clearLists[i]->Close(), "Failed to close clear list");
  }
  return true;
}

void D3D12RenderBackend::RTWaitForFence(ID3D12Fence *fence, UINT64 value) {
  if (fence->GetCompletedValue() == UINT64_MAX) {
    RTGetDeviceRemovedReason();
    return;
  }
  if (fence->GetCompletedValue() >= value) return;
  const auto result = fence->SetEventOnCompletion(value, _fenceEvent);
  if (FAILED(result) && FAILED(RTGetDeviceRemovedReason())) return;
  Check(result, "Failed to set fence event");
  while (WaitForSingleObject(_fenceEvent, 1000) == WAIT_TIMEOUT) {
    if (FAILED(RTGetDeviceRemovedReason())) return;
  }
  if (fence->GetCompletedValue() < value)
    FATALERROR(this, "Failed to wait for GPU fence");
}

void D3D12RenderBackend::RTWaitForGpuIdle() {
  if (!_idleFence || FAILED(RTGetDeviceRemovedReason())) return;
  // Wait each signal before reusing this fence on another queue.
  for (auto queue :
       {_directQueue.Get(), _computeQueue.Get(), _copyQueue.Get()}) {
    const auto result = queue->Signal(_idleFence, ++_idleValue);
    if (FAILED(result) && FAILED(RTGetDeviceRemovedReason())) return;
    Check(result, "Failed to signal idle fence");
    RTWaitForFence(_idleFence, _idleValue);
    if (FAILED(RTGetDeviceRemovedReason())) return;
  }
}

void D3D12RenderBackend::RTReleaseBackBuffers() {
  _frame = {};
  for (auto &buffer : _backBuffers) buffer.Clear();
}

void D3D12RenderBackend::RTReleaseSwapChain() {
  _streamline.RTSuspendFrameGeneration();
  RTReleaseBackBuffers();
  if (_frameWaitable) {
    CloseHandle(_frameWaitable);
    _frameWaitable = nullptr;
  }
  _nativeSwapChain.Clear();
  _swapChain.Clear();
}

void D3D12RenderBackend::RTUninit(bool exclusiveMode) {
  RTWaitForGpuIdle();
  if (_swapChain && exclusiveMode) {
    BOOL fullscreen = FALSE;
    if (SUCCEEDED(_nativeSwapChain->GetFullscreenState(&fullscreen, nullptr)) &&
        fullscreen) {
      _streamline.RTSuspendFrameGeneration();
      _swapChain->SetFullscreenState(FALSE, nullptr);
    }
  }
  RTReleaseSwapChain();
  for (auto &list : _clearLists) list.Clear();
  for (auto &allocator : _clearAllocators) allocator.Clear();
  _rtvHeap.Clear();
  _frameFence.Clear();
  _idleFence.Clear();
  _proxyDirectQueue.Clear();
  _directQueue.Clear();
  _computeQueue.Clear();
  _copyQueue.Clear();
  _adapter.Clear();
  if (_fenceEvent) {
    CloseHandle(_fenceEvent);
    _fenceEvent = nullptr;
  }
  // After every proxy is released and before the device is released.
  _streamline.RTShutdown();
#if defined(_DEBUG)
  // The report goes through the message callback into the log; the debug
  // device itself keeps one device reference.
  auto debug = _device.As<ID3D12DebugDevice>();
  _device.Clear();
  if (debug) {
    debug->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL |
                                   D3D12_RLDO_IGNORE_INTERNAL);
    if (_hasDebugCallback) {
      debug.As<ID3D12InfoQueue1>()->UnregisterMessageCallback(_debugCallback);
      _hasDebugCallback = false;
    }
  }
#else
  _device.Clear();
#endif
}

void D3D12RenderBackend::RTCreateSwapChain(HWND window,
                                           const DXGI_SWAP_CHAIN_DESC1 &desc,
                                           DXGI_COLOR_SPACE_TYPE colorSpace,
                                           std::optional<UINT> maxFrameLatency,
                                           UINT32 generatedFrames) {
  RTWaitForGpuIdle();
  RTReleaseSwapChain();
  _swapDesc = desc;
  _colorSpace = colorSpace;
  _swapDesc.BufferCount = BackBufferCount;
  _swapDesc.SampleDesc = {1, 0};
  _swapDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  // Frame pacing: without frame generation the frame starts when the swap
  // chain latency waitable signals, as it does without Streamline; low latency
  // then also sleeps in Streamline at the frame start. With frame generation
  // Streamline presents on its own thread and owns the waitable of a swap
  // chain made without the flag, so the frame waits only for the frame fence
  // and the Streamline sleep.
  const bool frameGeneration =
      _streamline.RTPrepareSwapChain(generatedFrames, _swapDesc.Format);
  if (frameGeneration)
    _swapDesc.Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  else
    _swapDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  ComObject<IDXGIFactory2> factory;
  Check(_adapter->GetParent(IID_PPV_ARGS(factory.Assign())),
        "Failed to get device factory");
  const auto proxyFactory = _streamline.RTUpgradeFactory(factory);
  ComObject<IDXGISwapChain1> chain;
  Check((proxyFactory ? proxyFactory : factory)
            ->CreateSwapChainForHwnd(
                _proxyDirectQueue ? _proxyDirectQueue : _directQueue, window,
                &_swapDesc, nullptr, nullptr, chain.Assign()),
        "Failed to create D3D12 swapchain");
  _swapChain = chain.As<IDXGISwapChain3>();
  _nativeSwapChain = proxyFactory
                         ? _streamline.RTGetNative<IDXGISwapChain3>(_swapChain)
                         : _swapChain;
  if (!_nativeSwapChain) FATALERROR(this, "Failed to get native swapchain");
  if (!frameGeneration) {
    Check(_nativeSwapChain->SetMaximumFrameLatency(
              maxFrameLatency.value_or(D3D12FrameState::FramesInFlight)),
          "Failed to set frame latency");
    _frameWaitable = _nativeSwapChain->GetFrameLatencyWaitableObject();
    if (!_frameWaitable) FATALERROR(this, "Failed to get frame latency handle");
  }
  LOG("D3D12 swapchain created, frame generation "
          << (frameGeneration ? "on" : "off"),
      MGDF_LOG_LOW);
  Check(factory->MakeWindowAssociation(
            window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES),
        "Failed to disable DXGI alt-enter");
}

HRESULT D3D12RenderBackend::RTResizeBackBuffer(
    const DXGI_SWAP_CHAIN_DESC1 &desc) {
  RTWaitForGpuIdle();
  RTReleaseBackBuffers();
  _swapDesc.Width = desc.Width;
  _swapDesc.Height = desc.Height;
  _streamline.RTSuspendFrameGeneration();
  const auto result =
      _swapChain->ResizeBuffers(BackBufferCount, desc.Width, desc.Height,
                                DXGI_FORMAT_UNKNOWN, _swapDesc.Flags);
  _streamline.RTResumeFrameGeneration();
  if (FAILED(result)) return result;
  if (!SetSwapChainColorSpace(_nativeSwapChain, _colorSpace)) {
    LOG("Swapchain does not support color space " << _colorSpace,
        MGDF_LOG_ERROR);
  }
  RTSetHDRMetadata(_hdrMetadata);
  auto handle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
  for (UINT i = 0; i < BackBufferCount; ++i) {
    Check(_swapChain->GetBuffer(i, IID_PPV_ARGS(_backBuffers[i].Assign())),
          "Failed to get backbuffer");
    _device->CreateRenderTargetView(_backBuffers[i], nullptr, handle);
    handle.ptr += _rtvSize;
  }
  LOG("D3D12 backbuffer: " << desc.Width << "x" << desc.Height, MGDF_LOG_LOW);
  return S_OK;
}

void D3D12RenderBackend::RTSetHDRMetadata(
    const std::optional<DXGI_HDR_METADATA_HDR10> &metadata) {
  _hdrMetadata = metadata;
  if (!_nativeSwapChain) return;
  const auto result =
      SetSwapChainHDRMetadata(_nativeSwapChain, _colorSpace, _hdrMetadata);
  if (FAILED(result))
    LOG("Failed to set HDR metadata " << result, MGDF_LOG_ERROR);
  else if (result == S_OK)
    LOG("HDR metadata MaxCLL: "
            << (_hdrMetadata
                    ? std::to_string(_hdrMetadata->MaxContentLightLevel)
                    : "none"),
        MGDF_LOG_LOW);
}

bool D3D12RenderBackend::RTSetExclusiveFullscreen() {
  ComObject<IDXGIOutput> primary;

  if (FAILED(_adapter->EnumOutputs(0, primary.Assign()))) return false;
  _streamline.RTSuspendFrameGeneration();
  const auto result = _swapChain->SetFullscreenState(TRUE, primary);
  if (FAILED(result)) {
    _streamline.RTResumeFrameGeneration();
    LOG("Exclusive fullscreen is not available: " << result, MGDF_LOG_LOW);
    return false;
  }
  const DXGI_MODE_DESC target{.Width = _swapDesc.Width,
                              .Height = _swapDesc.Height,
                              .RefreshRate = {0, 0},
                              .Format = _swapDesc.Format};
  const bool resized = SUCCEEDED(_nativeSwapChain->ResizeTarget(&target));
  if (!resized) {
    LOG("Cannot change exclusive display resolution", MGDF_LOG_ERROR);
    _swapChain->SetFullscreenState(FALSE, nullptr);
  }
  _streamline.RTResumeFrameGeneration();
  return resized;
}

void D3D12RenderBackend::RTSetWindowed() {
  if (!_swapChain) return;
  _streamline.RTSuspendFrameGeneration();
  Check(_swapChain->SetFullscreenState(FALSE, nullptr),
        "Failed to leave exclusive fullscreen");
  _streamline.RTResumeFrameGeneration();
}

bool D3D12RenderBackend::RTWaitForFrame(MGDFLowLatencyMode lowLatency) {
  if (_frameWaitable) {
    const auto result = WaitForSingleObject(_frameWaitable, 1000);
    if (result == WAIT_TIMEOUT || FAILED(RTGetDeviceRemovedReason()))
      return false;
    if (result != WAIT_OBJECT_0)
      FATALERROR(this, "Failed to wait for swapchain");
  }
  RTWaitForFence(_frameFence, _frames.PreviousFence());
  _streamline.RTBeginFrame(lowLatency);
  const UINT index = _swapChain->GetCurrentBackBufferIndex();
  auto handle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
  handle.ptr += static_cast<SIZE_T>(index) * _rtvSize;
  _frame = {_frames.Ordinal(),
            _frameFence->GetCompletedValue(),
            _frames.Slot(),
            D3D12FrameState::FramesInFlight,
            index,
            _backBuffers[index],
            handle,
            _backBuffers[index]->GetDesc()};
  return true;
}

void D3D12RenderBackend::RTClear() {
  auto &allocator = _clearAllocators[_frame.FrameSlot];
  auto &list = _clearLists[_frame.FrameSlot];
  Check(allocator->Reset(), "Failed to reset clear allocator");
  Check(list->Reset(allocator, nullptr), "Failed to reset clear list");
  D3D12_RESOURCE_BARRIER barrier{
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
      .Transition = {.pResource = _frame.BackBuffer,
                     .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                     .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
                     .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET}};
  list->ResourceBarrier(1, &barrier);
  const float black[4] = {0, 0, 0, 1};
  list->ClearRenderTargetView(_frame.BackBufferRTV, black, 0, nullptr);
  std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
  list->ResourceBarrier(1, &barrier);
  Check(list->Close(), "Failed to close clear list");
  ID3D12CommandList *lists[] = {list};
  _directQueue->ExecuteCommandLists(1, lists);
}

HRESULT D3D12RenderBackend::RTPresent(UINT syncInterval, UINT flags) {
  syncInterval = _streamline.RTGetSyncInterval(syncInterval);
  if (!(_swapDesc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) || syncInterval)
    flags &= ~DXGI_PRESENT_ALLOW_TEARING;
  _streamline.RTBeforePresent();
  const auto result = _swapChain->Present(syncInterval, flags);
  _streamline.RTAfterPresent();
  return result;
}

HRESULT D3D12RenderBackend::RTEndFrame() {
  const auto result = _directQueue->Signal(_frameFence, _frames.Ordinal());
  if (SUCCEEDED(result)) _frames.Submitted();
  return result;
}

HRESULT D3D12RenderBackend::RTGetDeviceRemovedReason() const {
  if (!_device) return S_OK;
  const auto reason = _device->GetDeviceRemovedReason();
  if (FAILED(reason)) {
    LOG("D3D12 device removed: " << std::hex << reason, MGDF_LOG_ERROR);
    ComObject<ID3D12DeviceRemovedExtendedData1> dred;
    if (SUCCEEDED(_device->QueryInterface(IID_PPV_ARGS(dred.Assign())))) {
      D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 breadcrumbs{};
      if (SUCCEEDED(dred->GetAutoBreadcrumbsOutput1(&breadcrumbs))) {
        for (auto node = breadcrumbs.pHeadAutoBreadcrumbNode; node;
             node = node->pNext) {
          LOG("DRED list " << (node->pCommandListDebugNameA
                                   ? node->pCommandListDebugNameA
                                   : "unnamed")
                           << ": completed "
                           << (node->pLastBreadcrumbValue
                                   ? *node->pLastBreadcrumbValue
                                   : 0)
                           << " of " << node->BreadcrumbCount,
              MGDF_LOG_ERROR);
        }
      }
      D3D12_DRED_PAGE_FAULT_OUTPUT1 fault{};
      if (SUCCEEDED(dred->GetPageFaultAllocationOutput1(&fault))) {
        LOG("DRED page fault VA: " << std::hex << fault.PageFaultVA,
            MGDF_LOG_ERROR);
        for (auto head : {fault.pHeadExistingAllocationNode,
                          fault.pHeadRecentFreedAllocationNode}) {
          for (auto node = head; node; node = node->pNext)
            LOG("DRED allocation: "
                    << (node->ObjectNameA ? node->ObjectNameA : "unnamed")
                    << ", type " << node->AllocationType,
                MGDF_LOG_ERROR);
        }
      }
    }
  }
  return reason;
}

bool D3D12RenderBackend::RTIsInitialized() const {
  return static_cast<bool>(_device);
}
ComObject<IUnknown> D3D12RenderBackend::RTGetDevice() {
  return _device.As<IUnknown>();
}
ComObject<IUnknown> D3D12RenderBackend::RTGetBackBuffer() { return {}; }
ComObject<IUnknown> D3D12RenderBackend::RTGetDepthStencilBuffer() { return {}; }
ComObject<IDXGIAdapter> D3D12RenderBackend::RTGetAdapter() { return _adapter; }
MGDFBackBufferInfo D3D12RenderBackend::RTGetBackBufferInfo() const {
  return {_swapDesc.Width, _swapDesc.Height, _swapDesc.Format, 1};
}
ComObject<ID3D12CommandQueue> D3D12RenderBackend::RTGetQueue(
    D3D12_COMMAND_LIST_TYPE type) {
  switch (type) {
    case D3D12_COMMAND_LIST_TYPE_DIRECT:
      return _directQueue;
    case D3D12_COMMAND_LIST_TYPE_COMPUTE:
      return _computeQueue;
    case D3D12_COMMAND_LIST_TYPE_COPY:
      return _copyQueue;
    default:
      return {};
  }
}
ComObject<ID3D12Fence> D3D12RenderBackend::RTGetFrameFence() {
  return _frameFence;
}
MGDFFrameInfo D3D12RenderBackend::RTGetCurrentFrame() const { return _frame; }
Streamline *D3D12RenderBackend::GetStreamline() { return &_streamline; }

}  // namespace core
}  // namespace MGDF
