#include "StdAfx.h"

#include "MGDFD3DAppFramework.hpp"

#include <math.h>
#include <mmsystem.h>
#include <string.h>

#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFResources.hpp"
#include "windowsx.h"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

// Compute the overlay area of two rectangles, A and B.
// (ax1, ay1) = left-top coordinates of A; (ax2, ay2) = right-bottom coordinates
// of A (bx1, by1) = left-top coordinates of B; (bx2, by2) = right-bottom
// coordinates of B
constexpr int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2,
                                      int bx1, int by1, int bx2, int by2) {
  return max(0, min(ax2, bx2) - max(ax1, bx1)) *
         max(0, min(ay2, by2) - max(ay1, by1));
}

#define WINDOW_CLASS_NAME "MGDFD3DAppFrameworkWindowClass"

D3DAppFramework::D3DAppFramework(HINSTANCE hInstance)
    : _applicationInstance(hInstance),
      _window(nullptr),
      _maximized(false),
#if defined(DEBUG) || defined(_DEBUG)
      _factoryFlags(DXGI_CREATE_FACTORY_DEBUG),
#else
      _factoryFlags(0U),
#endif
      _adapterIndex(0U),
      _resizing(false),
      _awaitingResize(false),
      _renderThread(nullptr),
      _internalShutDown(false),
      _windowStyle(WS_OVERLAPPEDWINDOW),
      _allowTearing(false),
      _frameWaitableObject(nullptr) {
  _minimized.store(false);
  _resize.store(false);
  _runRenderThread.clear();

  SecureZeroMemory(&_clientOffset, sizeof(POINT));
  SecureZeroMemory(&_currentFullScreen, sizeof(MGDFFullScreenDesc));
  SecureZeroMemory(&_windowRect, sizeof(RECT));
  SecureZeroMemory(&_currentSize, sizeof(POINT));
  SecureZeroMemory(&_swapDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
  SecureZeroMemory(&_fullscreenSwapDesc,
                   sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
}

D3DAppFramework::~D3DAppFramework() {
  if (_window != nullptr) {
    UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(nullptr));
  }

  UninitD3D();
}

void D3DAppFramework::InitWindow(const std::string &caption,
                                 WNDPROC windowProcedure) {
  // if the window has not already been created
  if (!_window) {
    LOG("Initializing window...", MGDF_LOG_LOW);
    const WNDCLASS wc{
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = windowProcedure,
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = _applicationInstance,
        .hIcon = LoadIcon(0, IDI_APPLICATION),
        .hCursor = LoadCursor(0, IDC_ARROW),
        .hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH),
        .lpszMenuName = 0,
        .lpszClassName = WINDOW_CLASS_NAME,
    };

    if (!RegisterClass(&wc)) {
      FATALERROR(this, "RegisterClass FAILED");
    }

    _windowStyle = WS_OVERLAPPEDWINDOW;
    if (!OnInitWindow(_windowRect)) {
      _windowStyle &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    }

    const INT32 x = _windowRect.left;
    const INT32 y = _windowRect.top;
    _windowRect.bottom -= _windowRect.top;
    _windowRect.right -= _windowRect.left;
    _windowRect.top = 0;
    _windowRect.left = 0;

    if (_windowRect.right < static_cast<LONG>(Resources::MIN_SCREEN_X)) {
      _windowRect.right = Resources::MIN_SCREEN_X;
    }
    if (_windowRect.bottom < static_cast<LONG>(Resources::MIN_SCREEN_Y)) {
      _windowRect.bottom = Resources::MIN_SCREEN_Y;
    }

    if (!AdjustWindowRect(&_windowRect, _windowStyle, false)) {
      FATALERROR(this, "AdjustWindowRect FAILED");
    }
    _clientOffset.x = abs(_windowRect.left);
    _clientOffset.y = abs(_windowRect.top);

    const INT32 width = _windowRect.right - _windowRect.left;
    const INT32 height = _windowRect.bottom - _windowRect.top;

    _window = CreateWindow(WINDOW_CLASS_NAME, caption.c_str(), _windowStyle,
                           CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0,
                           _applicationInstance, 0);

    if (!_window) {
      FATALERROR(this, "CreateWindow FAILED");
    }

    if (x || y) {
      const POINT pt{.x = x, .y = y};
      HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

      MONITORINFO monitorInfo;
      memset(&monitorInfo, 0, sizeof(MONITORINFO));
      monitorInfo.cbSize = sizeof(MONITORINFO);
      if (!GetMonitorInfo(monitor, &monitorInfo)) {
        FATALERROR(this, "GetMonitorInfo failed");
      }

      SetWindowPos(
          _window, HWND_NOTOPMOST,
          min(max(x, monitorInfo.rcWork.left), monitorInfo.rcWork.right),
          min(max(y, monitorInfo.rcWork.top), monitorInfo.rcWork.bottom), 0, 0,
          SWP_NOSIZE);
    }

    ShowWindow(_window, SW_SHOW);

    if (!GetWindowRect(_window, &_windowRect)) {
      FATALERROR(this, "GetWindowRect failed");
    }

    InitRawInput();

    LOG("Getting compatible D3D feature levels...", MGDF_LOG_LOW);
    UINT64 levelsSize = 0;
    if (GetCompatibleD3DFeatureLevels(nullptr, &levelsSize)) {
      _levels.resize(levelsSize);
      GetCompatibleD3DFeatureLevels(_levels.data(), &levelsSize);
    }

    InitD3D();
  }
}

void D3DAppFramework::InitRawInput() {
  LOG("Initializing Raw Input...", MGDF_LOG_LOW);
  const RAWINPUTDEVICE Rid[2] = {
      {
          .usUsagePage = 0x01,  // desktop input
          .usUsage = 0x02,      // mouse
          .dwFlags = 0,
          .hwndTarget = _window,
      },
      {
          .usUsagePage = 0x01,         // desktop input
          .usUsage = 0x06,             // keyboard
          .dwFlags = RIDEV_NOHOTKEYS,  // disable windows key and other windows
                                       // hotkeys while the game has focus
          .hwndTarget = _window,
      }};

  if (!RegisterRawInputDevices(Rid, 2, sizeof(Rid[0]))) {
    FATALERROR(this,
               "Failed to register raw input devices for mouse and keyboard");
  }
}

void D3DAppFramework::InitD3D() {
  LOG("Initializing Direct3D...", MGDF_LOG_LOW);
#if defined(DEBUG) || defined(_DEBUG)
  constexpr const UINT32 createDeviceFlags =
      D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#else
  constexpr const UINT32 createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#endif

  if (FAILED(
          CreateDXGIFactory2(_factoryFlags, IID_PPV_ARGS(_factory.Assign())))) {
    FATALERROR(this, "Failed to create IDXGIFactory6.");
  }

  BOOL allowTearing = FALSE;
  if (FAILED(_factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                           &allowTearing,
                                           sizeof(allowTearing)))) {
    allowTearing = FALSE;
  }
  _allowTearing = allowTearing == TRUE;

  ComObject<IDXGIAdapter1> adapter;
  ComObject<IDXGIAdapter1> bestAdapter;
  SIZE_T bestAdapterMemory = 0;

  char videoCardDescription[128];
  SecureZeroMemory(videoCardDescription, sizeof(videoCardDescription));
  DXGI_ADAPTER_DESC1 adapterDesc = {};

  // step through the adapters and ensure we use the best one to create our
  // device
  LOG("Enumerating display adapters...", MGDF_LOG_LOW);
  for (INT32 i = 0;
       _factory->EnumAdapters1(i, adapter.Assign()) != DXGI_ERROR_NOT_FOUND;
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

    if (!bestAdapter || adapterDesc.DedicatedVideoMemory > bestAdapterMemory) {
      D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
      ComObject<ID3D11Device> device;
      ComObject<ID3D11DeviceContext> context;

      if (SUCCEEDED(D3D11CreateDevice(
              adapter,
              D3D_DRIVER_TYPE_UNKNOWN,  // as we're specifying an adapter to
                                        // use, we must specify that the driver
                                        // type is unknown!!!
              0,                        // no software device
              createDeviceFlags, _levels.data(),
              static_cast<UINT>(_levels.size()),  // default feature level array
              D3D11_SDK_VERSION, device.Assign(), &featureLevel,
              context.Assign())) &&
          featureLevel != 0) {
        // this is the first acceptable adapter, or the best one so far
        if (!_d3dDevice || featureLevel >= _d3dDevice->GetFeatureLevel()) {
          // store the new best adapter
          bestAdapter = adapter;
          bestAdapterMemory = adapterDesc.DedicatedVideoMemory;
          _adapterIndex = i;
          _d3dDevice = device;
          _immediateContext = context;
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

  if (!_d3dDevice) {
    FATALERROR(this,
               "No adapters found supporting The specified D3D Feature set");
  } else {
    LOG("Created device with D3D Feature level: "
            << _d3dDevice->GetFeatureLevel(),
        MGDF_LOG_LOW);
  }

  const D2D1_FACTORY_OPTIONS options {
#if defined(DEBUG) || defined(_DEBUG)
    .debugLevel = D2D1_DEBUG_LEVEL_INFORMATION
#else
    .debugLevel = D2D1_DEBUG_LEVEL_NONE
#endif
  };

  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, options,
                               _d2dFactory.Assign()))) {
    FATALERROR(this, "Unable to create ID2DFactory1");
  }

  ComObject<IDXGIDevice1> dxgiDevice;
  if (FAILED(_d3dDevice->QueryInterface<IDXGIDevice1>(dxgiDevice.Assign()))) {
    FATALERROR(this, "Unable to acquire IDXGIDevice from ID3D11Device");
  }

  if (FAILED(_d2dFactory->CreateDevice(dxgiDevice, _d2dDevice.Assign()))) {
    FATALERROR(this, "Unable to create ID2D1Device");
  }

  OnInitDevices(_window, _d3dDevice, _d2dDevice, bestAdapter);
  CheckForDisplayChanges();

  RECT windowSize;
  if (!GetClientRect(_window, &windowSize)) {
    FATALERROR(this, "GetClientRect failed");
  }
  OnBeforeBackBufferChange();
  _currentFullScreen =
      OnResetSwapChain(_swapDesc, _fullscreenSwapDesc, windowSize);
  CreateSwapChain();
  ResizeBackBuffer();
}

void D3DAppFramework::PrepareToReinitD3D() {
  const HRESULT reason = _d3dDevice->GetDeviceRemovedReason();
  LOG("Device removed! DXGI_ERROR code " << reason, MGDF_LOG_ERROR);

  _awaitingD3DReset.store(true);
  OnBeforeDeviceReset();
  UninitD3D();
}

void D3DAppFramework::UninitD3D() {
  LOG("Cleaning up Direct3D resources...", MGDF_LOG_LOW);
  if (_immediateContext) {
    _immediateContext->ClearState();
    _immediateContext->Flush();
  }

  if (_swapChain && _currentFullScreen.ExclusiveMode) {
    BOOL fullscreen = false;
    if (FAILED(_swapChain->GetFullscreenState(&fullscreen, nullptr)) &&
        fullscreen) {
      // d3d has to be in windowed mode to cleanup correctly
      _swapChain->SetFullscreenState(false, nullptr);
    }
  }

  _backBuffer.Clear();
  _renderTargetView.Clear();
  _depthStencilView.Clear();
  _depthStencilBuffer.Clear();
  _swapChain.Clear();
  _factory.Clear();
  _immediateContext.Clear();
  _d2dDevice.Clear();
  _d2dFactory.Clear();

  if (_d3dDevice) {
#if defined(_DEBUG)
    ComObject<ID3D11Debug> debug;
    const bool failed =
        FAILED(_d3dDevice->QueryInterface<ID3D11Debug>(debug.Assign()));
#endif
    _d3dDevice.Clear();
#if defined(_DEBUG)
    if (!failed) {
      debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL |
                                     D3D11_RLDO_IGNORE_INTERNAL);
    }
#endif
  }
}

void D3DAppFramework::QueueResetDevice() { _awaitingD3DReset.store(false); }

void D3DAppFramework::ReinitD3D() {
  InitD3D();
  OnDeviceReset();
}

bool D3DAppFramework::AllowTearing() {
  return (
      _allowTearing && !VSyncEnabled() &&
      !(_currentFullScreen.FullScreen && _currentFullScreen.ExclusiveMode) &&
      _swapDesc.SampleDesc.Count == 1);
}

void D3DAppFramework::CreateSwapChain() {
  if (AllowTearing()) {
    _swapDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }

  // ensure everything referencing the old swapchain is cleaned up
  ClearBackBuffer();
  _immediateContext->ClearState();
  _immediateContext->Flush();

  LOG("Creating swapchain...", MGDF_LOG_LOW);
  if (FAILED(_factory->CreateSwapChainForHwnd(_d3dDevice, _window, &_swapDesc,
                                              nullptr, nullptr,
                                              _swapChain.Assign()))) {
    FATALERROR(this, "Failed to create swap chain");
  }

  if (_swapDesc.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT) {
    _frameWaitableObject =
        _swapChain.As<IDXGISwapChain2>()->GetFrameLatencyWaitableObject();
  } else {
    _frameWaitableObject = nullptr;
  }
  OnSwapChainCreated(_swapChain);

  if (FAILED(_factory->MakeWindowAssociation(
          _window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES))) {
    FATALERROR(this, "Failed to disable alt-enter");
  }
}

void D3DAppFramework::ClearBackBuffer() {
  // Release the old views, as they hold references to the buffers we
  // will be destroying.  Also release the old depth/stencil buffer.
  _backBuffer.Clear();
  _renderTargetView.Clear();
  _depthStencilView.Clear();
  _depthStencilBuffer.Clear();
}

void D3DAppFramework::ResizeBackBuffer() {
  ID3D11RenderTargetView *nullRTView = nullptr;
  _immediateContext->OMSetRenderTargets(1, &nullRTView, nullptr);

  ClearBackBuffer();

  LOG("Setting backbuffer to " << _swapDesc.Width << "x" << _swapDesc.Height,
      MGDF_LOG_MEDIUM);

  _currentSize.x = _swapDesc.Width;
  _currentSize.y = _swapDesc.Height;

  const HRESULT result =
      _swapChain->ResizeBuffers(0, _swapDesc.Width, _swapDesc.Height,
                                DXGI_FORMAT_UNKNOWN, _swapDesc.Flags);

  // Resize the swap chain and recreate the render target view.
  if (result == DXGI_ERROR_DEVICE_REMOVED ||
      result == DXGI_ERROR_DEVICE_RESET) {
    PrepareToReinitD3D();
    return;
  } else if (FAILED(result)) {
    FATALERROR(this, "Failed to resize swapchain buffers");
  }

  if (FAILED(_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
                                   (void **)_backBuffer.Assign()))) {
    FATALERROR(this, "Failed to get swapchain buffer");
  }
  if (FAILED(_d3dDevice->CreateRenderTargetView(_backBuffer, 0,
                                                _renderTargetView.Assign()))) {
    FATALERROR(this, "Failed to create render target view from backbuffer");
  }

  // Create the depth/stencil buffer and view.
  const D3D11_TEXTURE2D_DESC depthStencilDesc{
      .Width = _swapDesc.Width,
      .Height = _swapDesc.Height,
      .MipLevels = 1,
      .ArraySize = 1,
      .Format = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .SampleDesc =
          {
              .Count = _swapDesc.SampleDesc.Count,
              .Quality = _swapDesc.SampleDesc.Quality,
          },
      .Usage = D3D11_USAGE_DEFAULT,
      .BindFlags = D3D11_BIND_DEPTH_STENCIL,
      .CPUAccessFlags = 0,
      .MiscFlags = 0,
  };

  if (FAILED(_d3dDevice->CreateTexture2D(&depthStencilDesc, 0,
                                         _depthStencilBuffer.Assign()))) {
    FATALERROR(this, "Failed to create texture from depth stencil description");
  }

  if (FAILED(_d3dDevice->CreateDepthStencilView(_depthStencilBuffer, 0,
                                                _depthStencilView.Assign()))) {
    FATALERROR(this,
               "Failed to create depthStencilView from depth stencil buffer");
  }

  // Bind the render target view and depth/stencil view to the pipeline.
  _immediateContext->OMSetRenderTargets(1, _renderTargetView.AsArray(),
                                        _depthStencilView);

  // Set the viewport transform.
  const D3D11_VIEWPORT viewPort{
      .TopLeftX = 0,
      .TopLeftY = 0,
      .Width = static_cast<float>(_swapDesc.Width),
      .Height = static_cast<float>(_swapDesc.Height),
      .MinDepth = 0.0f,
      .MaxDepth = 1.0f,
  };

  _immediateContext->RSSetViewports(1, &viewPort);

  OnBackBufferChange(_backBuffer, _depthStencilBuffer);
}

INT32 D3DAppFramework::Run() {
  // if the window or d3d has not been initialised, quit with an error
  if (!_window && !_d3dDevice) {
    return -1;
  }

  std::atomic_flag runSimThread;
  _runRenderThread.test_and_set();

  // run the simulation in its own thread
  std::thread simThread([this, &runSimThread]() {
    runSimThread.test_and_set();

    LOG("Starting sim thread...", MGDF_LOG_LOW);
    while (runSimThread.test_and_set()) {
      OnUpdateSim();
    }
    LOG("Stopping sim thread...", MGDF_LOG_LOW);
  });

  // run the renderer in its own thread
  _renderThread = std::make_unique<std::thread>([this]() {
    LOG("Starting render thread...", MGDF_LOG_LOW);
    OnBeforeFirstDraw();

    while (_runRenderThread.test_and_set()) {
      if (!_factory->IsCurrent()) {
        LOG("DXGI factory is no longer current, rechecking...", MGDF_LOG_LOW);
        CheckForDisplayChanges();
      }
      bool exp = true;
      const bool awaitingReset = _awaitingD3DReset.load();

      if (awaitingReset) {
        // waiting for the module to signal that its cleaned up all
        // its D3D resources and is ready for a device reset
        Sleep(100);
        continue;
      }

      if (!_d3dDevice) {
        if (!awaitingReset) {
          LOG("Reinitializing D3D after Device Reset...", MGDF_LOG_MEDIUM);
          ReinitD3D();
        } else {
          continue;
        }
      } else if (IsBackBufferChangePending()) {
        // the game logic step may force the device to reset, so lets check
        LOG("Module has scheduled a backbuffer change...", MGDF_LOG_LOW);

        RECT windowSize;
        if (!GetClientRect(_window, &windowSize)) {
          FATALERROR(this, "GetClientRect failed");
        }

        OnBeforeBackBufferChange();
        const MGDFFullScreenDesc newFullScreen =
            OnResetSwapChain(_swapDesc, _fullscreenSwapDesc, windowSize);

        if (_swapChain && _currentFullScreen.ExclusiveMode) {
          // clean up the old swap chain, then recreate it with the new
          // settings
          BOOL fullscreen = false;
          if (FAILED(_swapChain->GetFullscreenState(&fullscreen, nullptr))) {
            FATALERROR(this, "GetFullscreenState failed");
          }
          if (fullscreen) {
            // d3d has to be in windowed mode to cleanup correctly
            if (FAILED(_swapChain->SetFullscreenState(false, nullptr))) {
              FATALERROR(this, "SetFullscreenState failed");
            }
          }
        }
        _currentFullScreen = newFullScreen;

        if (!newFullScreen.ExclusiveMode) {
          // switch to fullscreen-borderless
          if (newFullScreen.FullScreen) {
            if (!GetWindowRect(_window, &_windowRect)) {
              FATALERROR(this, "GetWindowRect failed");
            }
            if (!SetWindowLongW(_window, GWL_STYLE,
                                WS_OVERLAPPEDWINDOW &
                                    ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                                      WS_MINIMIZEBOX | WS_MAXIMIZEBOX))) {
              FATALERROR(this, "SetWindowLongW failed");
            }
            HMONITOR hMonitor =
                MonitorFromWindow(_window, MONITOR_DEFAULTTONEAREST);
            MONITORINFOEX monitorInfo = {};
            monitorInfo.cbSize = sizeof(MONITORINFOEX);
            if (!GetMonitorInfo(hMonitor, &monitorInfo)) {
              FATALERROR(this, "GetMonitorInfo failed");
            }
            if (!SetWindowPos(
                    _window, HWND_TOP, monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.top,
                    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE)) {
              FATALERROR(this, "SetWindowPos failed");
            }
            ShowWindow(_window, SW_MAXIMIZE);
          } else {
            if (!SetWindowLong(_window, GWL_STYLE, _windowStyle)) {
              FATALERROR(this, "SetWindowLong failed");
            }
            if (!SetWindowPos(_window, HWND_NOTOPMOST, _windowRect.left,
                              _windowRect.top,
                              _windowRect.right - _windowRect.left,
                              _windowRect.bottom - _windowRect.top,
                              SWP_FRAMECHANGED | SWP_NOACTIVATE)) {
              FATALERROR(this, "SetWindowPos failed");
            }
            ShowWindow(_window, SW_NORMAL);
          }
        }
        CreateSwapChain();

        // reset the swap chain to fullscreen if it was previously fullscreen
        // or if this backbuffer change was for a toggle from windowed to
        // fullscreen
        if (newFullScreen.FullScreen && newFullScreen.ExclusiveMode) {
          // exclusive fullscreen is only supported on the primary output
          // so we need to fetch that before restoring the fullscreen state
          ComObject<IDXGIAdapter1> adapter;
          if (FAILED(
                  _factory->EnumAdapters1(_adapterIndex, adapter.Assign()))) {
            FATALERROR(this, "Failed to get adapter " << _adapterIndex
                                                      << " from factory");
          }
          ComObject<IDXGIOutput> primary;
          if (FAILED(adapter->EnumOutputs(0, primary.Assign()))) {
            FATALERROR(this, "Failed to get primary output");
          }
          if (FAILED(_swapChain->SetFullscreenState(true, primary))) {
            FATALERROR(this, "SetFullscreenState failed on primary output");
          }
        }
        ResizeBackBuffer();
      }
      // a window event may also have triggered a resize event.
      else if (_resize.compare_exchange_strong(exp, false)) {
        LOG("Resizing...", MGDF_LOG_MEDIUM);
        CheckForDisplayChanges();
        OnBeforeBackBufferChange();
        OnResize(_swapDesc.Width, _swapDesc.Height);
        ResizeBackBuffer();
      }

      if (!_minimized.load() && _d3dDevice) {
        if (_frameWaitableObject) {
          const DWORD wait =
              WaitForSingleObjectEx(_frameWaitableObject, 1000, true);
          if (wait == WAIT_ABANDONED || wait == WAIT_TIMEOUT ||
              wait == WAIT_FAILED) {
            LOG("Failed to wait on FrameWaitableObject", MGDF_LOG_ERROR);
          }
        }

        const float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // RGBA
        _immediateContext->ClearRenderTargetView(_renderTargetView, &black[0]);
        _immediateContext->ClearDepthStencilView(
            _depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f,
            0);

        OnDraw();

        HRESULT result = S_OK;
        {
          result = _swapChain->Present(
              VSyncEnabled() ? 1 : 0,
              AllowTearing() ? DXGI_PRESENT_ALLOW_TEARING : 0);
        }

        if (result == DXGI_ERROR_DEVICE_REMOVED ||
            result == DXGI_ERROR_DEVICE_RESET) {
          PrepareToReinitD3D();
        } else if (FAILED(result)) {
          FATALERROR(this, "Direct3d Present1 failed");
        } else if (_swapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD ||
                   _swapDesc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL) {
          // using flip modes means we need to re-bind the backbuffer to a
          // render target after each present
          _immediateContext->OMSetRenderTargets(1, _renderTargetView.AsArray(),
                                                _depthStencilView);
        }
      }
    }
    LOG("Stopping render thread...", MGDF_LOG_LOW);
  });

  MSG msg{
      .message = WM_NULL,
  };
  LOG("Starting input loop...", MGDF_LOG_LOW);

  while (GetMessage(&msg, _window, 0, 0) > 0) {
    // deal with any windows messages on the main thread, this allows us
    // to ensure that any user input is handled with as little latency as
    // possible independent of the update rate for the sim and render threads.
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  LOG("Stopping input loop...", MGDF_LOG_LOW);

  runSimThread.clear();
  simThread.join();

  return (int)msg.wParam;
}

void D3DAppFramework::CloseWindow() {
  LOG("Sending WM_CLOSE message...", MGDF_LOG_HIGH);
  _internalShutDown = true;
  PostMessage(_window, WM_CLOSE, 0, 0);
}

void D3DAppFramework::CheckForDisplayChanges() {
  if (!_factory) {
    return;
  }

  if (!_factory->IsCurrent()) {
    LOG("DXGI factory is no longer current, recreating...", MGDF_LOG_LOW);
    if (FAILED(CreateDXGIFactory2(_factoryFlags,
                                  IID_PPV_ARGS(_factory.Assign())))) {
      FATALERROR(this, "Failed to create IDXGIFactory6.");
    }
  }

  ComObject<IDXGIAdapter1> adapter;
  if (FAILED(_factory->EnumAdapters1(_adapterIndex, adapter.Assign()))) {
    FATALERROR(this,
               "Failed to get adapter " << _adapterIndex << " from factory");
  }

  UINT i = 0;
  ComObject<IDXGIOutput> currentOutput;
  ComObject<IDXGIOutput6> bestOutput;
  ComObject<IDXGIOutput6> primaryOutput;
  float bestIntersectArea = -1;

  // the best matching output is the one with the largest intersection area
  // with the app window
  LOG("Checking outputs to find best match for current window...",
      MGDF_LOG_HIGH);
  while (adapter->EnumOutputs(i, currentOutput.Assign()) !=
         DXGI_ERROR_NOT_FOUND) {
    // Get the rectangle bounds of current output
    DXGI_OUTPUT_DESC desc;
    if (FAILED(currentOutput->GetDesc(&desc))) {
      FATALERROR(this, "Failed to get description from output " << i);
    }
    const RECT r = desc.DesktopCoordinates;
    const int bx1 = r.left;
    const int by1 = r.top;
    const int bx2 = r.right;
    const int by2 = r.bottom;

    const int intersectArea = ComputeIntersectionArea(
        _windowRect.left, _windowRect.top, _windowRect.right,
        _windowRect.bottom, bx1, by1, bx2, by2);
    if (intersectArea > bestIntersectArea) {
      LOG("Found matching output (["
              << bx1 << "," << by1 << "]->[" << bx2 << "," << by2
              << "]) for current window ([" << _windowRect.left << ","
              << _windowRect.top << "]->[" << _windowRect.right << ","
              << _windowRect.bottom << "])...",
          MGDF_LOG_HIGH);
      bestOutput = currentOutput.As<IDXGIOutput6>();
      bestIntersectArea = static_cast<float>(intersectArea);
    }

    if (i == 0) {
      primaryOutput = currentOutput.As<IDXGIOutput6>();
    }

    ++i;
  }

  if (!bestOutput || !primaryOutput) {
    FATALERROR(this, "No outputs found");
  }

  DXGI_OUTPUT_DESC1 primaryDesc;
  if (FAILED(primaryOutput->GetDesc1(&primaryDesc))) {
    FATALERROR(
        this,
        "Failed to get description from primary output overlapping window");
  }

  UINT32 maxSDRAdaptorModes = 0U;
  primaryOutput->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0,
                                     &maxSDRAdaptorModes, nullptr);

  UINT32 maxHDRAdaptorModes = 0U;
  if (primaryDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
    LOG("Primary output supports HDR, getting SDR & HDR compatible modes",
        MGDF_LOG_LOW);
    primaryOutput->GetDisplayModeList1(DXGI_FORMAT_R16G16B16A16_FLOAT, 0,
                                       &maxHDRAdaptorModes, nullptr);
  } else {
    LOG("Primary output is SDR only, getting SDR compatible modes only",
        MGDF_LOG_LOW);
  }

  std::vector<DXGI_MODE_DESC1> primaryModes(
      static_cast<size_t>(maxSDRAdaptorModes) +
      static_cast<size_t>(maxHDRAdaptorModes));

  // get all valid modes for the outputs supported
  if (FAILED(primaryOutput->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0,
                                                &maxSDRAdaptorModes,
                                                primaryModes.data())) ||
      (maxHDRAdaptorModes > 0U &&
       FAILED(primaryOutput->GetDisplayModeList1(
           DXGI_FORMAT_R16G16B16A16_FLOAT, 0, &maxHDRAdaptorModes,
           primaryModes.data() + maxSDRAdaptorModes)))) {
    FATALERROR(this, "Failed to get mode lists from adapter");
  }

  primaryModes.erase(
      std::remove_if(
          primaryModes.begin(), primaryModes.end(),
          [](const DXGI_MODE_DESC1 &mode) {
            return mode.Scaling != DXGI_MODE_SCALING_UNSPECIFIED ||
                   mode.Stereo ||  // Stereo adapters not currently supported
                   mode.Width < Resources::MIN_SCREEN_X ||
                   mode.Height < Resources::MIN_SCREEN_Y;
          }),
      primaryModes.end());

  DXGI_OUTPUT_DESC1 currentDesc;
  if (FAILED(bestOutput->GetDesc1(&currentDesc))) {
    FATALERROR(this,
               "Failed to get description from output overlapping window");
  }

  MONITORINFOEXW monitorInfo{};
  monitorInfo.cbSize = sizeof(MONITORINFOEXW);
  GetMonitorInfoW(currentDesc.Monitor, &monitorInfo);

  ULONG currentSDRWhiteLevel = 1000U;

  // get additional info from the monitor that DXGI doesn't provide
  // such as the SDR reference white level and DPI
  for (LONG result = ERROR_INSUFFICIENT_BUFFER;
       result == ERROR_INSUFFICIENT_BUFFER;) {
    uint32_t pathElements, modeElements;
    if (::GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathElements,
                                      &modeElements) != ERROR_SUCCESS) {
      break;
    }
    std::vector<DISPLAYCONFIG_PATH_INFO> pathInfos(pathElements);
    std::vector<DISPLAYCONFIG_MODE_INFO> modeInfos(modeElements);
    result = ::QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathElements,
                                  pathInfos.data(), &modeElements,
                                  modeInfos.data(), nullptr);
    if (result == ERROR_SUCCESS) {
      pathInfos.resize(pathElements);

      for (auto &path : pathInfos) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME deviceName = {};
        deviceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        deviceName.header.size = sizeof(deviceName);
        deviceName.header.adapterId = path.sourceInfo.adapterId;
        deviceName.header.id = path.sourceInfo.id;
        // check if this path matches the output we are interested in
        if ((DisplayConfigGetDeviceInfo(&deviceName.header) == ERROR_SUCCESS) &&
            (wcscmp(monitorInfo.szDevice, deviceName.viewGdiDeviceName) == 0)) {
          // query the reference SDR white level
          DISPLAYCONFIG_SDR_WHITE_LEVEL whiteLevel = {};
          whiteLevel.header.type =
              DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
          whiteLevel.header.size = sizeof(whiteLevel);
          whiteLevel.header.adapterId = path.targetInfo.adapterId;
          whiteLevel.header.id = path.targetInfo.id;
          if (DisplayConfigGetDeviceInfo(&whiteLevel.header) == ERROR_SUCCESS) {
            currentSDRWhiteLevel = whiteLevel.SDRWhiteLevel;
          }
        }
      }
    }
  }

  const UINT currentDPI = GetDpiForWindow(_window);

  LOG("Current display changed (HDR: "
          << (currentDesc.ColorSpace ==
                      DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020
                  ? "true"
                  : "false")
          << ", DPI: " << currentDPI
          << ", SDRWhiteLevel: " << currentSDRWhiteLevel << ")",
      MGDF_LOG_LOW);

  OnDisplayChange(currentDesc, currentDPI, currentSDRWhiteLevel, primaryModes);
}

LRESULT D3DAppFramework::MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam,
                                 LPARAM lParam) {
  OnBeforeHandleMessage();

  switch (msg) {
    // handle player mouse input
    case WM_MOUSEMOVE: {
      const INT32 x = GET_X_LPARAM(lParam);
      const INT32 y = GET_Y_LPARAM(lParam);
      OnMouseInput(x, y);
      return 0;
    }

    // Handle player keyboard input
    case WM_INPUT: {
      UINT32 dwSize = 0U;
      GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
                      sizeof(RAWINPUTHEADER));
      std::vector<BYTE> lpb(dwSize);
      if (lpb.size()) {
        const UINT32 readSize =
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize,
                            sizeof(RAWINPUTHEADER));

        if (readSize != dwSize) {
          FATALERROR(this, "GetRawInputData returned incorrect size");
        } else {
          RAWINPUT *rawInput = (RAWINPUT *)lpb.data();
          OnRawInput(rawInput);
        }
      }
      // Even if you handle the event, you have to call DefWindowProx after a
      // WM_Input message so the  can perform cleanup
      // http://msdn.microsoft.com/en-us/library/windows/desktop/ms645590%28v=vs.85%29.aspx
      return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    case WM_DISPLAYCHANGE:
      CheckForDisplayChanges();
      return 0;

    case WM_DPICHANGED: {
      // unlike a Windows GUI app we don't want to resize the window if the DPI
      // changes as we want to respect the resolution selected by the user for
      // performance reasons but we do want to notify the module of the change
      // so it can adjust its rendering of text and other elements that are DPI
      // sensitive
      CheckForDisplayChanges();
      return 0;
    }

    // WM_SIZE is sent when the user resizes the window.
    case WM_SIZE:
      _swapDesc.Width = LOWORD(lParam);
      _swapDesc.Height = HIWORD(lParam);

      if (wParam == SIZE_MINIMIZED) {
        _minimized.store(true);
      } else if (wParam == SIZE_MAXIMIZED) {
        _minimized.store(false);
        _resize.store(true);
      } else if (wParam == SIZE_RESTORED) {
        // Restored is any resize that is not a minimize or maximize.
        // For example, restoring the window to its default size
        // after a minimize or maximize, or from dragging the resize
        // bars.
        bool exp = true;
        if (!_swapChain || _resizing ||
            _minimized.compare_exchange_strong(exp, false)) {
          // Don't resize until the user has finished resizing or if we
          // are just starting up and haven't initialized d3d yet, or we
          // are simply restoring the window view without changing the size
        } else {
          _resize.store(true);
        }
      }
      return 0;

    // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
      _resizing = true;
      return 0;

    // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
      _resizing = false;
      if (_currentSize.x != static_cast<LONG>(_swapDesc.Width) ||
          _currentSize.y != static_cast<LONG>(_swapDesc.Height)) {
        if (!GetWindowRect(_window, &_windowRect)) {
          FATALERROR(this, "GetWindowRect failed");
        }
        _resize.store(true);
      }
      return 0;

    // Don't allow window sizes smaller than the min required screen
    // resolution
    case WM_GETMINMAXINFO: {
      PMINMAXINFO info = (PMINMAXINFO)lParam;
      info->ptMinTrackSize.x = Resources::MIN_SCREEN_X;
      info->ptMinTrackSize.y = Resources::MIN_SCREEN_Y;
      return 0;
    }

    case WM_MOVE: {
      if (!_currentFullScreen.FullScreen) {
        if (!GetWindowRect(_window, &_windowRect)) {
          FATALERROR(this, "GetWindowRect failed");
        }
        CheckForDisplayChanges();
        OnMoveWindow((int)(short)LOWORD(lParam) - _clientOffset.x,
                     (int)(short)HIWORD(lParam) - _clientOffset.y);
      }
      return 0;
    }

    // WM_CLOSE is sent when the user presses the 'X' button in the
    // caption bar menu, when the host schedules a shutdown
    case WM_CLOSE:
      if (_internalShutDown) {
        if (_renderThread) {
          // make sure we stop rendering before disposing of the window
          _runRenderThread.clear();
          _renderThread->join();
        }
        // if we triggered this, then shut down
        DestroyWindow(_window);
      } else {
        // otherwise just inform the rest of the system that
        // it should shut down ASAP, but give it time to shut down cleanly
        OnExternalClose();
      }
      return 0;

    // WM_DESTROY is sent when the window is being destroyed.
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;

    // Don't beep when we alt-enter.
    case WM_MENUCHAR:
      return MAKELRESULT(0, MNC_CLOSE);

    default:
      return OnHandleMessage(hwnd, msg, wParam, lParam);
  }
}

}  // namespace core
}  // namespace MGDF
