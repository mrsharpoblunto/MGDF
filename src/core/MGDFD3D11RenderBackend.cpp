#include "StdAfx.h"

#include "MGDFD3D11RenderBackend.hpp"

#include "common/MGDFLoggerImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

D3D11RenderBackend::D3D11RenderBackend(
    const ComObject<IDXGIFactory6> &factory,
    std::vector<D3D_FEATURE_LEVEL> levels,
    std::function<void(const char *, const char *)> fatalError)
    : _rtFactory(factory),
      _rtLevels(std::move(levels)),
      _fatalError(std::move(fatalError)),
      _rtFrameWaitableObject(nullptr) {}

bool D3D11RenderBackend::RTInit() {
#if defined(DEBUG) || defined(_DEBUG)
  constexpr const UINT32 createDeviceFlags =
      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#else
  constexpr const UINT32 createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif

  if (!_rtFactory) {
    // use the default adapter to create the device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (FAILED(::D3D11CreateDevice(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags,
            _rtLevels.data(), static_cast<UINT>(_rtLevels.size()),
            D3D11_SDK_VERSION, _rtD3dDevice.Assign(), &featureLevel,
            _rtImmediateContext.Assign())) ||
        featureLevel == 0) {
      LOG("Failed to create device with default adapter", MGDF_LOG_ERROR);
      return false;
    }
  } else {
    // step through the adapters and ensure we use the best one to create our
    // device
    ComObject<IDXGIAdapter1> adapter;
    ComObject<IDXGIAdapter1> bestAdapter;
    SIZE_T bestAdapterMemory = 0;

    char videoCardDescription[128];
    ::SecureZeroMemory(videoCardDescription, sizeof(videoCardDescription));
    DXGI_ADAPTER_DESC1 adapterDesc = {};

    LOG("Enumerating display adapters...", MGDF_LOG_LOW);
    for (INT32 i = 0;
         _rtFactory->EnumAdapters1(i, adapter.Assign()) != DXGI_ERROR_NOT_FOUND;
         i++) {
      adapter->GetDesc1(&adapterDesc);

      if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
        LOG("Skipping software adapter", MGDF_LOG_LOW);
        continue;
      }

      size_t length = wcslen(adapterDesc.Description);
#if defined(_DEBUG) || defined(DEBUG)
      size_t stringLength = 0;
      const INT32 error = wcstombs_s(&stringLength, videoCardDescription, 128,
                                     adapterDesc.Description, length);
      _ASSERTE(!error);
#endif

      std::string message(videoCardDescription, videoCardDescription + length);
      message.insert(0, "Attempting to create device for adapter ");
      LOG(message, MGDF_LOG_LOW);

      if (!bestAdapter ||
          adapterDesc.DedicatedVideoMemory > bestAdapterMemory) {
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        ComObject<ID3D11Device> device;
        ComObject<ID3D11DeviceContext> context;

        if (SUCCEEDED(::D3D11CreateDevice(
                adapter,
                D3D_DRIVER_TYPE_UNKNOWN,  // as we're specifying an adapter to
                                          // use, we must specify that the
                                          // driver type is unknown!!!
                0,                        // no software device
                createDeviceFlags, _rtLevels.data(),
                static_cast<UINT>(
                    _rtLevels.size()),  // default feature level array
                D3D11_SDK_VERSION, device.Assign(), &featureLevel,
                context.Assign())) &&
            featureLevel != 0) {
          // this is the first acceptable adapter, or the best one so far
          if (!_rtD3dDevice ||
              featureLevel >= _rtD3dDevice->GetFeatureLevel()) {
            // store the new best adapter
            bestAdapter = adapter;
            bestAdapterMemory = adapterDesc.DedicatedVideoMemory;
            _rtD3dDevice = device;
            _rtImmediateContext = context;
            LOG("Adapter is the best found so far", MGDF_LOG_LOW);
          }
          // this adapter is no better than what we already have, so ignore it
          else {
            LOG("A better adapter has already been found - Ignoring",
                MGDF_LOG_LOW);
          }
        }
      }
    }
  }

  if (!_rtD3dDevice) {
    LOG("No adapters found supporting the specified D3D feature set",
        MGDF_LOG_ERROR);
    return false;
  } else {
    LOG("Created device with D3D Feature level: "
            << _rtD3dDevice->GetFeatureLevel(),
        MGDF_LOG_LOW);
  }

  return true;
}

void D3D11RenderBackend::RTSetExclusiveFullscreen() {
  // exclusive fullscreen is only supported on the primary output
  LOG("Switching to exclusive fullscreen on primary output", MGDF_LOG_LOW);
  ComObject<IDXGIDevice> device = _rtD3dDevice.As<IDXGIDevice>();
  ComObject<IDXGIAdapter> adapter;
  device->GetAdapter(adapter.Assign());
  ComObject<IDXGIOutput> primary;
  if (FAILED(adapter->EnumOutputs(0, primary.Assign()))) {
    FATALERROR(this, "Failed to get primary output");
  }
  if (FAILED(_rtSwapChain->SetFullscreenState(true, primary))) {
    FATALERROR(this, "SetFullscreenState failed on primary output");
  }
}

void D3D11RenderBackend::RTUninit(bool exclusiveMode) {
  if (_rtImmediateContext) {
    _rtImmediateContext->ClearState();
    _rtImmediateContext->Flush();
  }

  if (_rtSwapChain && exclusiveMode) {
    BOOL fullscreen = false;
    if (FAILED(_rtSwapChain->GetFullscreenState(&fullscreen, nullptr)) &&
        fullscreen) {
      // d3d has to be in windowed mode to cleanup correctly
      _rtSwapChain->SetFullscreenState(false, nullptr);
    }
  }

  _rtBackBuffer.Clear();
  _rtRenderTargetView.Clear();
  _rtDepthStencilView.Clear();
  _rtDepthStencilBuffer.Clear();
  _rtSwapChain.Clear();
  _rtImmediateContext.Clear();

  if (_rtD3dDevice) {
#if defined(_DEBUG)
    ComObject<ID3D11Debug> debug;
    const bool failed =
        FAILED(_rtD3dDevice->QueryInterface<ID3D11Debug>(debug.Assign()));
#endif
    _rtD3dDevice.Clear();
#if defined(_DEBUG)
    if (!failed) {
      debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL |
                                     D3D11_RLDO_IGNORE_INTERNAL);
    }
#endif
  }
}

ComObject<IDXGIAdapter> D3D11RenderBackend::RTGetAdapter() {
  ComObject<IDXGIDevice1> dxgiDevice;
  if (FAILED(_rtD3dDevice->QueryInterface<IDXGIDevice1>(dxgiDevice.Assign()))) {
    FATALERROR(this, "Unable to acquire IDXGIDevice from ID3D11Device");
  }
  ComObject<IDXGIAdapter> adapter;
  dxgiDevice->GetAdapter(adapter.Assign());

  return adapter;
}

void D3D11RenderBackend::RTCreateSwapChain(
    HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc,
    std::optional<UINT> maxFrameLatency) {
  _rtSwapDesc = desc;
  // ensure everything referencing the old swapchain is cleaned up
  RTClearBackBuffer();
  _rtImmediateContext->ClearState();
  _rtImmediateContext->Flush();

  // don't use the member _rtFactory as that may have been created later
  // and isn't associated with the d3d device
  ComObject<IDXGIDevice> dxgiDevice;
  if (FAILED(_rtD3dDevice->QueryInterface<IDXGIDevice>(dxgiDevice.Assign()))) {
    FATALERROR(this, "Unable to acquire IDXGIDevice from ID3D11Device");
  }
  ComObject<IDXGIAdapter> adapter;
  dxgiDevice->GetAdapter(adapter.Assign());
  ComObject<IDXGIFactory2> factory;
  adapter->GetParent(IID_PPV_ARGS(factory.Assign()));

  LOG("Creating swapchain...", MGDF_LOG_LOW);
  if (FAILED(factory->CreateSwapChainForHwnd(_rtD3dDevice, window, &_rtSwapDesc,
                                             nullptr, nullptr,
                                             _rtSwapChain.Assign()))) {
    FATALERROR(this, "Failed to create swap chain");
  }

  if (_rtSwapDesc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) {
    _rtFrameWaitableObject =
        _rtSwapChain.As<IDXGISwapChain2>()->GetFrameLatencyWaitableObject();
  } else {
    _rtFrameWaitableObject = nullptr;
  }
  if (maxFrameLatency) {
    _rtSwapChain.As<IDXGISwapChain2>()->SetMaximumFrameLatency(
        *maxFrameLatency);
  }

  if (FAILED(factory->MakeWindowAssociation(
          window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES))) {
    FATALERROR(this, "Failed to disable alt-enter");
  }
}

void D3D11RenderBackend::RTClearBackBuffer() {
  // Release the old views, as they hold references to the buffers we
  // will be destroying.  Also release the old depth/stencil buffer.
  _rtBackBuffer.Clear();
  _rtRenderTargetView.Clear();
  _rtDepthStencilView.Clear();
  _rtDepthStencilBuffer.Clear();
}

HRESULT D3D11RenderBackend::RTResizeBackBuffer(
    const DXGI_SWAP_CHAIN_DESC1 &desc) {
  _rtSwapDesc = desc;
  ID3D11RenderTargetView *nullRTView = nullptr;
  _rtImmediateContext->OMSetRenderTargets(1, &nullRTView, nullptr);

  RTClearBackBuffer();

  LOG("Setting backbuffer to " << _rtSwapDesc.Width << "x"
                               << _rtSwapDesc.Height,
      MGDF_LOG_MEDIUM);

  const HRESULT result =
      _rtSwapChain->ResizeBuffers(0, _rtSwapDesc.Width, _rtSwapDesc.Height,
                                  DXGI_FORMAT_UNKNOWN, _rtSwapDesc.Flags);

  if (FAILED(result)) {
    return result;
  }

  if (FAILED(_rtSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                     (void **)_rtBackBuffer.Assign()))) {
    FATALERROR(this, "Failed to get swapchain buffer");
  }
  if (FAILED(_rtD3dDevice->CreateRenderTargetView(
          _rtBackBuffer, 0, _rtRenderTargetView.Assign()))) {
    FATALERROR(this, "Failed to create render target view from backbuffer");
  }

  // Create the depth/stencil buffer and view.
  const D3D11_TEXTURE2D_DESC depthStencilDesc{
      .Width = _rtSwapDesc.Width,
      .Height = _rtSwapDesc.Height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .SampleDesc =
          {
              .Count = _rtSwapDesc.SampleDesc.Count,
              .Quality = _rtSwapDesc.SampleDesc.Quality,
          },
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_DEPTH_STENCIL,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
  };

  if (FAILED(_rtD3dDevice->CreateTexture2D(&depthStencilDesc, 0,
                                           _rtDepthStencilBuffer.Assign()))) {
    FATALERROR(this, "Failed to create texture from depth stencil description");
  }

  if (FAILED(_rtD3dDevice->CreateDepthStencilView(
          _rtDepthStencilBuffer, 0, _rtDepthStencilView.Assign()))) {
    FATALERROR(this,
               "Failed to create depthStencilView from depth stencil buffer");
  }

  // Bind the render target view and depth/stencil view to the pipeline.
  _rtImmediateContext->OMSetRenderTargets(1, _rtRenderTargetView.AsArray(),
                                          _rtDepthStencilView);

  // Set the viewport transform.
  const D3D11_VIEWPORT viewPort{
      .TopLeftX = 0,
      .TopLeftY = 0,
      .Width = static_cast<float>(_rtSwapDesc.Width),
      .Height = static_cast<float>(_rtSwapDesc.Height),
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f,
  };

  _rtImmediateContext->RSSetViewports(1, &viewPort);

  return result;
}

void D3D11RenderBackend::RTSetWindowed() {
  if (_rtSwapChain) {
    // clean up the old swap chain, then recreate it with the new
    // settings
    BOOL fullscreen = false;
    if (FAILED(_rtSwapChain->GetFullscreenState(&fullscreen, nullptr))) {
      FATALERROR(this, "GetFullscreenState failed");
    }
    if (fullscreen) {
      LOG("Switching from exclusive fullscreen to windowed mode", MGDF_LOG_LOW);
      // d3d has to be in windowed mode to cleanup correctly
      if (FAILED(_rtSwapChain->SetFullscreenState(false, nullptr))) {
        FATALERROR(this, "SetFullscreenState failed");
      }
    }
  }
}

void D3D11RenderBackend::RTWaitForFrame() {
  if (_rtFrameWaitableObject) {
    const DWORD wait =
        ::WaitForSingleObjectEx(_rtFrameWaitableObject, 1000, true);
    if (wait == WAIT_ABANDONED || wait == WAIT_TIMEOUT || wait == WAIT_FAILED) {
      LOG("Failed to wait on FrameWaitableObject", MGDF_LOG_ERROR);
    }
  }
}

void D3D11RenderBackend::RTClear() {
  const float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // RGBA
  _rtImmediateContext->ClearRenderTargetView(_rtRenderTargetView, &black[0]);
  _rtImmediateContext->ClearDepthStencilView(
      _rtDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

HRESULT D3D11RenderBackend::RTPresent(UINT syncInterval, UINT flags) {
  const HRESULT result = _rtSwapChain->Present(syncInterval, flags);
  if (SUCCEEDED(result) &&
      (_rtSwapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
       _rtSwapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL)) {
    // using flip modes means we need to re-bind the backbuffer to a
    // render target after each present
    _rtImmediateContext->OMSetRenderTargets(1, _rtRenderTargetView.AsArray(),
                                            _rtDepthStencilView);
  }
  return result;
}

void D3D11RenderBackend::RTWaitForGpuIdle() {
  if (_rtImmediateContext) {
    _rtImmediateContext->Flush();
  }
}

bool D3D11RenderBackend::RTIsInitialized() const {
  return static_cast<bool>(_rtD3dDevice);
}

HRESULT D3D11RenderBackend::RTGetDeviceRemovedReason() const {
  return _rtD3dDevice->GetDeviceRemovedReason();
}

ComObject<IUnknown> D3D11RenderBackend::RTGetDevice() {
  return _rtD3dDevice.As<IUnknown>();
}

ComObject<IUnknown> D3D11RenderBackend::RTGetBackBuffer() {
  return _rtBackBuffer.As<IUnknown>();
}

ComObject<IUnknown> D3D11RenderBackend::RTGetDepthStencilBuffer() {
  return _rtDepthStencilBuffer.As<IUnknown>();
}

MGDFBackBufferInfo D3D11RenderBackend::RTGetBackBufferInfo() const {
  D3D11_TEXTURE2D_DESC desc{};
  if (_rtBackBuffer) _rtBackBuffer->GetDesc(&desc);
  return {desc.Width, desc.Height, desc.Format, desc.SampleDesc.Count};
}

void D3D11RenderBackend::FatalError(const char *sender, const char *message) {
  _fatalError(sender, message);
}

}  // namespace core
}  // namespace MGDF
