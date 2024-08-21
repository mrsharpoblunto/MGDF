#include "StdAfx.h"

#include "MGDFD3DAppFramework.hpp"

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
      _renderThread(nullptr),
      _internalShutDown(false),
      _windowStyle(WS_OVERLAPPEDWINDOW),
      _rtAllowTearing(false),
      _rtFrameWaitableObject(nullptr) {
  _minimized.store(false);
  _runRenderThread.clear();

  ::SecureZeroMemory(&_clientOffset, sizeof(POINT));
  ::SecureZeroMemory(&_rtWindowRect, sizeof(RECT));
  ::SecureZeroMemory(&_rtCurrentFullScreen, sizeof(MGDFFullScreenDesc));
  ::SecureZeroMemory(&_rtSwapDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
  ::SecureZeroMemory(&_rtFullscreenSwapDesc,
                     sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
}

D3DAppFramework::~D3DAppFramework() {
  if (_window != nullptr) {
    ::UnregisterClass(WINDOW_CLASS_NAME, GetModuleHandle(nullptr));
  }

  RTUninitD3D();
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

    if (!::RegisterClass(&wc)) {
      FATALERROR(this, "RegisterClass FAILED");
    }

    _windowStyle = WS_OVERLAPPEDWINDOW;
    if (!OnInitWindow(_rtWindowRect)) {
      _windowStyle &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    }

    const INT32 x = _rtWindowRect.left;
    const INT32 y = _rtWindowRect.top;
    _rtWindowRect.bottom -= _rtWindowRect.top;
    _rtWindowRect.right -= _rtWindowRect.left;
    _rtWindowRect.top = 0;
    _rtWindowRect.left = 0;

    if (_rtWindowRect.right < static_cast<LONG>(Resources::MIN_SCREEN_X)) {
      _rtWindowRect.right = Resources::MIN_SCREEN_X;
    }
    if (_rtWindowRect.bottom < static_cast<LONG>(Resources::MIN_SCREEN_Y)) {
      _rtWindowRect.bottom = Resources::MIN_SCREEN_Y;
    }

    if (!::AdjustWindowRect(&_rtWindowRect, _windowStyle, false)) {
      FATALERROR(this, "AdjustWindowRect FAILED");
    }
    _clientOffset.x = abs(_rtWindowRect.left);
    _clientOffset.y = abs(_rtWindowRect.top);

    const INT32 width = _rtWindowRect.right - _rtWindowRect.left;
    const INT32 height = _rtWindowRect.bottom - _rtWindowRect.top;

    _window = ::CreateWindow(WINDOW_CLASS_NAME, caption.c_str(), _windowStyle,
                             CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0,
                             _applicationInstance, 0);

    if (!_window) {
      FATALERROR(this, "CreateWindow FAILED");
    }

    if (x || y) {
      const POINT pt{.x = x, .y = y};
      HMONITOR monitor = ::MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

      MONITORINFO monitorInfo;
      ::memset(&monitorInfo, 0, sizeof(MONITORINFO));
      monitorInfo.cbSize = sizeof(MONITORINFO);
      if (!::GetMonitorInfo(monitor, &monitorInfo)) {
        FATALERROR(this, "GetMonitorInfo failed");
      }

      ::SetWindowPos(
          _window, HWND_NOTOPMOST,
          min(max(x, monitorInfo.rcWork.left), monitorInfo.rcWork.right),
          min(max(y, monitorInfo.rcWork.top), monitorInfo.rcWork.bottom), 0, 0,
          SWP_NOSIZE);
    }

    ::ShowWindow(_window, SW_SHOW);

    if (!::GetWindowRect(_window, &_rtWindowRect)) {
      FATALERROR(this, "GetWindowRect failed");
    }

    InitRawInput();

    LOG("Getting compatible D3D feature levels...", MGDF_LOG_LOW);
    UINT64 levelsSize = 0;
    if (GetCompatibleD3DFeatureLevels(nullptr, &levelsSize)) {
      _rtLevels.resize(levelsSize);
      GetCompatibleD3DFeatureLevels(_rtLevels.data(), &levelsSize);
    }

    RTInitD3D(_window);
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

  if (!::RegisterRawInputDevices(Rid, 2, sizeof(Rid[0]))) {
    FATALERROR(this,
               "Failed to register raw input devices for mouse and keyboard");
  }
}

void D3DAppFramework::RTInitD3D(const HWND window) {
  LOG("Initializing Direct3D...", MGDF_LOG_LOW);

  _rtFactory = RTCreateDXGIFactory();

  BOOL allowTearing = FALSE;
  if (FAILED(_rtFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                             &allowTearing,
                                             sizeof(allowTearing)))) {
    allowTearing = FALSE;
  }
  _rtAllowTearing = allowTearing == TRUE;

  ComObject<IDXGIAdapter1> adapter;
  ComObject<IDXGIAdapter1> bestAdapter;
  SIZE_T bestAdapterMemory = 0;

  char videoCardDescription[128];
  ::SecureZeroMemory(videoCardDescription, sizeof(videoCardDescription));
  DXGI_ADAPTER_DESC1 adapterDesc = {};

  // step through the adapters and ensure we use the best one to create our
  // device
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

    if (!bestAdapter || adapterDesc.DedicatedVideoMemory > bestAdapterMemory) {
      D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
      ComObject<ID3D11Device> device;
      ComObject<ID3D11DeviceContext> context;

#if defined(DEBUG) || defined(_DEBUG)
      constexpr const UINT32 createDeviceFlags =
          D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG;
#else
      constexpr const UINT32 createDeviceFlags =
          D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#endif

      if (SUCCEEDED(::D3D11CreateDevice(
              adapter,
              D3D_DRIVER_TYPE_UNKNOWN,  // as we're specifying an adapter to
                                        // use, we must specify that the driver
                                        // type is unknown!!!
              0,                        // no software device
              createDeviceFlags, _rtLevels.data(),
              static_cast<UINT>(
                  _rtLevels.size()),  // default feature level array
              D3D11_SDK_VERSION, device.Assign(), &featureLevel,
              context.Assign())) &&
          featureLevel != 0) {
        // this is the first acceptable adapter, or the best one so far
        if (!_rtD3dDevice || featureLevel >= _rtD3dDevice->GetFeatureLevel()) {
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

  if (!_rtD3dDevice) {
    FATALERROR(this,
               "No adapters found supporting The specified D3D Feature set");
  } else {
    LOG("Created device with D3D Feature level: "
            << _rtD3dDevice->GetFeatureLevel(),
        MGDF_LOG_LOW);
  }

  const D2D1_FACTORY_OPTIONS options {
#if defined(DEBUG) || defined(_DEBUG)
    .debugLevel = D2D1_DEBUG_LEVEL_INFORMATION
#else
    .debugLevel = D2D1_DEBUG_LEVEL_NONE
#endif
  };

  if (FAILED(::D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, options,
                                 _rtD2dFactory.Assign()))) {
    FATALERROR(this, "Unable to create ID2DFactory1");
  }

  ComObject<IDXGIDevice1> dxgiDevice;
  if (FAILED(_rtD3dDevice->QueryInterface<IDXGIDevice1>(dxgiDevice.Assign()))) {
    FATALERROR(this, "Unable to acquire IDXGIDevice from ID3D11Device");
  }

  if (FAILED(_rtD2dFactory->CreateDevice(dxgiDevice, _rtD2dDevice.Assign()))) {
    FATALERROR(this, "Unable to create ID2D1Device");
  }

  RTOnInitDevices(window, _rtD3dDevice, _rtD2dDevice, bestAdapter);
  RTCheckForDisplayChanges(window);

  RECT windowSize;
  if (!::GetClientRect(window, &windowSize)) {
    FATALERROR(this, "GetClientRect failed");
  }
  RTOnBeforeBackBufferChange();
  _rtCurrentFullScreen =
      RTOnResetSwapChain(_rtSwapDesc, _rtFullscreenSwapDesc, windowSize);
  RTCreateSwapChain(window);
  RTResizeBackBuffer();
}

ComObject<IDXGIFactory6> D3DAppFramework::RTCreateDXGIFactory() {
#if defined(DEBUG) || defined(_DEBUG)
  constexpr auto flags = DXGI_CREATE_FACTORY_DEBUG;
#else
  constexpr auto flags = 0U;
#endif
  ComObject<IDXGIFactory6> factory;
  if (FAILED(::CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.Assign())))) {
    FATALERROR(this, "Failed to create IDXGIFactory6.");
  }
  return factory;
}

void D3DAppFramework::RTPrepareToReinitD3D() {
  const HRESULT reason = _rtD3dDevice->GetDeviceRemovedReason();
  LOG("Device removed! DXGI_ERROR code " << reason, MGDF_LOG_ERROR);

  _awaitingD3DReset.store(true);
  RTOnBeforeDeviceReset();
  RTUninitD3D();
}

void D3DAppFramework::RTUninitD3D() {
  LOG("Cleaning up Direct3D resources...", MGDF_LOG_LOW);
  if (_rtImmediateContext) {
    _rtImmediateContext->ClearState();
    _rtImmediateContext->Flush();
  }

  if (_rtSwapChain && _rtCurrentFullScreen.ExclusiveMode) {
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
  _rtFactory.Clear();
  _rtImmediateContext.Clear();
  _rtD2dDevice.Clear();
  _rtD2dFactory.Clear();

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

void D3DAppFramework::RTCheckForDisplayChanges(const HWND window) {
  ComObject<IDXGIDevice1> dxgiDevice;
  if (FAILED(_rtD3dDevice->QueryInterface<IDXGIDevice1>(dxgiDevice.Assign()))) {
    FATALERROR(this, "Unable to acquire IDXGIDevice from ID3D11Device");
  }
  ComObject<IDXGIAdapter> adapter;
  dxgiDevice->GetAdapter(adapter.Assign());

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

    RECT windowRect;
    if (!::GetWindowRect(window, &windowRect)) {
      FATALERROR(this, "GetWindowRect failed");
    }

    const int intersectArea = ComputeIntersectionArea(
        windowRect.left, windowRect.top, windowRect.right, windowRect.bottom,
        bx1, by1, bx2, by2);
    if (intersectArea > bestIntersectArea) {
      LOG("Found matching output ([" << bx1 << "," << by1 << "]->[" << bx2
                                     << "," << by2 << "]) for current window (["
                                     << windowRect.left << "," << windowRect.top
                                     << "]->[" << windowRect.right << ","
                                     << windowRect.bottom << "])...",
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
    LOG("Primary output supports HDR", MGDF_LOG_LOW);
    primaryOutput->GetDisplayModeList1(DXGI_FORMAT_R16G16B16A16_FLOAT, 0,
                                       &maxHDRAdaptorModes, nullptr);
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
  ::GetMonitorInfoW(currentDesc.Monitor, &monitorInfo);

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
        if ((::DisplayConfigGetDeviceInfo(&deviceName.header) ==
             ERROR_SUCCESS) &&
            (::wcscmp(monitorInfo.szDevice, deviceName.viewGdiDeviceName) ==
             0)) {
          // query the reference SDR white level
          DISPLAYCONFIG_SDR_WHITE_LEVEL whiteLevel = {};
          whiteLevel.header.type =
              DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
          whiteLevel.header.size = sizeof(whiteLevel);
          whiteLevel.header.adapterId = path.targetInfo.adapterId;
          whiteLevel.header.id = path.targetInfo.id;
          if (::DisplayConfigGetDeviceInfo(&whiteLevel.header) ==
              ERROR_SUCCESS) {
            currentSDRWhiteLevel = whiteLevel.SDRWhiteLevel;
          }
        }
      }
    }
  }

  const UINT currentDPI = ::GetDpiForWindow(window);

  LOG("Current display changed (HDR: "
          << (currentDesc.ColorSpace ==
                      DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020
                  ? "true"
                  : "false")
          << ", DPI: " << currentDPI
          << ", SDRWhiteLevel: " << currentSDRWhiteLevel << ")",
      MGDF_LOG_LOW);

  RTOnDisplayChange(currentDesc, currentDPI, currentSDRWhiteLevel,
                    primaryModes);
}

void D3DAppFramework::RTReinitD3D(const HWND window) {
  RTInitD3D(window);
  RTOnDeviceReset();
}

bool D3DAppFramework::RTAllowTearing() {
  return (_rtAllowTearing && !RTVSyncEnabled() &&
          !(_rtCurrentFullScreen.FullScreen &&
            _rtCurrentFullScreen.ExclusiveMode) &&
          _rtSwapDesc.SampleDesc.Count == 1);
}

void D3DAppFramework::RTCreateSwapChain(const HWND window) {
  if (RTAllowTearing()) {
    _rtSwapDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }

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
  RTOnSwapChainCreated(_rtSwapChain);

  if (FAILED(factory->MakeWindowAssociation(
          window, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES))) {
    FATALERROR(this, "Failed to disable alt-enter");
  }
}

void D3DAppFramework::RTClearBackBuffer() {
  // Release the old views, as they hold references to the buffers we
  // will be destroying.  Also release the old depth/stencil buffer.
  _rtBackBuffer.Clear();
  _rtRenderTargetView.Clear();
  _rtDepthStencilView.Clear();
  _rtDepthStencilBuffer.Clear();
}

void D3DAppFramework::RTResizeBackBuffer() {
  ID3D11RenderTargetView *nullRTView = nullptr;
  _rtImmediateContext->OMSetRenderTargets(1, &nullRTView, nullptr);

  RTClearBackBuffer();

  LOG("Setting backbuffer to " << _rtSwapDesc.Width << "x"
                               << _rtSwapDesc.Height,
      MGDF_LOG_MEDIUM);

  const HRESULT result =
      _rtSwapChain->ResizeBuffers(0, _rtSwapDesc.Width, _rtSwapDesc.Height,
                                  DXGI_FORMAT_UNKNOWN, _rtSwapDesc.Flags);

  // Resize the swap chain and recreate the render target view.
  if (result == DXGI_ERROR_DEVICE_REMOVED ||
      result == DXGI_ERROR_DEVICE_RESET) {
    RTPrepareToReinitD3D();
    return;
  } else if (FAILED(result)) {
    FATALERROR(this, "Failed to resize swapchain buffers");
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

  RTOnBackBufferChange(_rtBackBuffer, _rtDepthStencilBuffer);
}

void D3DAppFramework::PushRTMessage(
    DisplayChangeType type,
    std::function<void(DisplayChangeMessage &)> genMessage) {
  std::lock_guard lock(_displayChangeMutex);
  auto message = &(_pendingDisplayChanges.empty()
                       ? _pendingDisplayChanges.emplace_back(type)
                       : _pendingDisplayChanges.back());
  if (message->Type != type) {
    message = &_pendingDisplayChanges.emplace_back(type);
  }
  if (genMessage) {
    genMessage(*message);
  }
}

bool D3DAppFramework::PopRTMessage(
    std::unique_ptr<DisplayChangeMessage> &message) {
  std::lock_guard<std::mutex> lock(_displayChangeMutex);
  if (!_pendingDisplayChanges.empty()) {
    message =
        std::make_unique<DisplayChangeMessage>(_pendingDisplayChanges.front());
    _pendingDisplayChanges.pop_front();
    return true;
  }
  return false;
}

void D3DAppFramework::QueueResetDevice() { _awaitingD3DReset.store(false); }

void D3DAppFramework::CloseWindow() {
  LOG("Sending WM_CLOSE message...", MGDF_LOG_HIGH);
  _internalShutDown = true;
  ::PostMessage(_window, WM_CLOSE, 0, 0);
}

INT32 D3DAppFramework::Run() {
  // if the window or d3d has not been initialised, quit with an error
  if (!_window && !_rtD3dDevice) {
    return -1;
  }

  std::atomic_flag runSimThread;
  _runRenderThread.test_and_set();

  // run the simulation in its own thread
  std::thread simThread([this, &runSimThread]() {
    runSimThread.test_and_set();

    LOG("Starting sim thread...", MGDF_LOG_LOW);
    while (runSimThread.test_and_set()) {
      STOnUpdateSim();
    }
    LOG("Stopping sim thread...", MGDF_LOG_LOW);
  });

  // run the renderer in its own thread
  _renderThread = std::make_unique<std::thread>(
      [this](const HWND window, const DWORD windowStyle) {
        LOG("Starting render thread...", MGDF_LOG_LOW);
        RTOnBeforeFirstDraw();

        while (_runRenderThread.test_and_set()) {
          const bool awaitingReset = _awaitingD3DReset.load();
          if (awaitingReset) {
            // waiting for the module to signal that its cleaned up all
            // its D3D resources and is ready for a device reset
            ::Sleep(100);
            continue;
          }

          if (!_rtD3dDevice) {
            if (!awaitingReset) {
              LOG("Reinitializing D3D after Device Reset...", MGDF_LOG_MEDIUM);
              RTReinitD3D(window);
            } else {
              continue;
            }
          } else {
            const auto dxgiFactoryIsCurrent = _rtFactory->IsCurrent();
            if (!dxgiFactoryIsCurrent) {
              LOG("DXGI factory is no longer current, recreating...",
                  MGDF_LOG_LOW);
              _rtFactory = RTCreateDXGIFactory();
            }

            // get the most recent display change message (if any)
            std::unique_ptr<DisplayChangeMessage> displayChange;
            if (PopRTMessage(displayChange) || !dxgiFactoryIsCurrent) {
              RTCheckForDisplayChanges(window);
            }

            // for window moves and resizes that don't result
            // in a fullscreen presentation, record the window
            // size so that restore to/from fullscreen works and
            // the window size is recorded for next startup
            if (displayChange.get() &&
                (displayChange->Type == DC_WINDOW_MOVE ||
                 displayChange->Type == DC_WINDOW_RESIZE) &&
                !_rtCurrentFullScreen.FullScreen) {
              if (!::GetWindowRect(window, &_rtWindowRect)) {
                FATALERROR(this, "GetWindowRect failed");
              }
            }

            // a window event may have triggered a resize event.
            if (displayChange && (displayChange->Type == DC_WINDOW_RESIZE ||
                                  displayChange->Type == DC_WINDOW_MAXIMIZE)) {
              LOG("Resizing...", MGDF_LOG_MEDIUM);
              _rtSwapDesc.Width = displayChange->Point.x;
              _rtSwapDesc.Height = displayChange->Point.y;
              RTOnBeforeBackBufferChange();
              RTOnResize(_rtSwapDesc.Width, _rtSwapDesc.Height);
              RTResizeBackBuffer();
            } else if (RTIsBackBufferChangePending()) {
              // the game logic step may force the device to reset, so lets
              // check
              LOG("Module has scheduled a backbuffer change...", MGDF_LOG_LOW);

              RECT windowSize;
              if (!::GetClientRect(window, &windowSize)) {
                FATALERROR(this, "GetClientRect failed");
              }

              RTOnBeforeBackBufferChange();
              const MGDFFullScreenDesc newFullScreen = RTOnResetSwapChain(
                  _rtSwapDesc, _rtFullscreenSwapDesc, windowSize);

              if (_rtSwapChain && _rtCurrentFullScreen.ExclusiveMode) {
                // clean up the old swap chain, then recreate it with the new
                // settings
                BOOL fullscreen = false;
                if (FAILED(_rtSwapChain->GetFullscreenState(&fullscreen,
                                                            nullptr))) {
                  FATALERROR(this, "GetFullscreenState failed");
                }
                if (fullscreen) {
                  // d3d has to be in windowed mode to cleanup correctly
                  if (FAILED(
                          _rtSwapChain->SetFullscreenState(false, nullptr))) {
                    FATALERROR(this, "SetFullscreenState failed");
                  }
                }
              }
              _rtCurrentFullScreen = newFullScreen;

              if (!newFullScreen.ExclusiveMode) {
                // switch to fullscreen-borderless
                if (newFullScreen.FullScreen) {
                  if (!::SetWindowLongW(
                          window, GWL_STYLE,
                          WS_OVERLAPPEDWINDOW &
                              ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                                WS_MINIMIZEBOX | WS_MAXIMIZEBOX))) {
                    FATALERROR(this, "SetWindowLongW failed");
                  }
                  HMONITOR hMonitor =
                      ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
                  MONITORINFOEX monitorInfo = {};
                  monitorInfo.cbSize = sizeof(MONITORINFOEX);
                  if (!::GetMonitorInfo(hMonitor, &monitorInfo)) {
                    FATALERROR(this, "GetMonitorInfo failed");
                  }
                  if (!::SetWindowPos(window, HWND_TOP,
                                      monitorInfo.rcMonitor.left,
                                      monitorInfo.rcMonitor.top,
                                      monitorInfo.rcMonitor.right -
                                          monitorInfo.rcMonitor.left,
                                      monitorInfo.rcMonitor.bottom -
                                          monitorInfo.rcMonitor.top,
                                      SWP_FRAMECHANGED | SWP_NOACTIVATE)) {
                    FATALERROR(this, "SetWindowPos failed");
                  }
                  ::ShowWindow(window, SW_MAXIMIZE);
                } else {
                  if (!::SetWindowLong(window, GWL_STYLE, windowStyle)) {
                    FATALERROR(this, "SetWindowLong failed");
                  }
                  if (!::SetWindowPos(window, HWND_NOTOPMOST,
                                      _rtWindowRect.left, _rtWindowRect.top,
                                      _rtWindowRect.right - _rtWindowRect.left,
                                      _rtWindowRect.bottom - _rtWindowRect.top,
                                      SWP_FRAMECHANGED | SWP_NOACTIVATE)) {
                    FATALERROR(this, "SetWindowPos failed");
                  }
                  ::ShowWindow(window, SW_NORMAL);
                }
              }
              RTCreateSwapChain(window);

              // reset the swap chain to fullscreen if it was previously
              // fullscreen or if this backbuffer change was for a toggle from
              // windowed to fullscreen
              if (newFullScreen.FullScreen && newFullScreen.ExclusiveMode) {
                // exclusive fullscreen is only supported on the primary output
                // so we need to fetch that before restoring the fullscreen
                // state
                ComObject<IDXGIDevice> device = _rtD3dDevice.As<IDXGIDevice>();
                ComObject<IDXGIAdapter> adapter;
                device->GetAdapter(adapter.Assign());
                ComObject<IDXGIOutput> primary;
                if (FAILED(adapter->EnumOutputs(0, primary.Assign()))) {
                  FATALERROR(this, "Failed to get primary output");
                }
                if (FAILED(_rtSwapChain->SetFullscreenState(true, primary))) {
                  FATALERROR(this,
                             "SetFullscreenState failed on primary output");
                }
              }
              RTResizeBackBuffer();
            }
          }

          if (!_minimized.load() && _rtD3dDevice) {
            if (_rtFrameWaitableObject) {
              const DWORD wait =
                  ::WaitForSingleObjectEx(_rtFrameWaitableObject, 1000, true);
              if (wait == WAIT_ABANDONED || wait == WAIT_TIMEOUT ||
                  wait == WAIT_FAILED) {
                LOG("Failed to wait on FrameWaitableObject", MGDF_LOG_ERROR);
              }
            }

            const float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};  // RGBA
            _rtImmediateContext->ClearRenderTargetView(_rtRenderTargetView,
                                                       &black[0]);
            _rtImmediateContext->ClearDepthStencilView(
                _rtDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                1.0f, 0);

            RTOnDraw();

            HRESULT result = S_OK;
            {
              result = _rtSwapChain->Present(
                  RTVSyncEnabled() ? 1 : 0,
                  RTAllowTearing() ? DXGI_PRESENT_ALLOW_TEARING : 0);
            }

            if (result == DXGI_ERROR_DEVICE_REMOVED ||
                result == DXGI_ERROR_DEVICE_RESET) {
              RTPrepareToReinitD3D();
            } else if (FAILED(result)) {
              FATALERROR(this, "Direct3d Present1 failed");
            } else if (_rtSwapDesc.SwapEffect ==
                           DXGI_SWAP_EFFECT_FLIP_DISCARD ||
                       _rtSwapDesc.SwapEffect ==
                           DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL) {
              // using flip modes means we need to re-bind the backbuffer to a
              // render target after each present
              _rtImmediateContext->OMSetRenderTargets(
                  1, _rtRenderTargetView.AsArray(), _rtDepthStencilView);
            }
          }
        }
        LOG("Stopping render thread...", MGDF_LOG_LOW);
      },
      _window, _windowStyle);

  MSG msg{
      .message = WM_NULL,
  };
  LOG("Starting input loop...", MGDF_LOG_LOW);

  while (::GetMessage(&msg, _window, 0, 0) > 0) {
    // deal with any windows messages on the main thread, this allows us
    // to ensure that any user input is handled with as little latency as
    // possible independent of the update rate for the sim and render threads.
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  LOG("Stopping input loop...", MGDF_LOG_LOW);

  runSimThread.clear();
  simThread.join();

  return (int)msg.wParam;
}

LRESULT D3DAppFramework::MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam,
                                 LPARAM lParam) {
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
      ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize,
                        sizeof(RAWINPUTHEADER));
      std::vector<BYTE> lpb(dwSize);
      if (lpb.size()) {
        const UINT32 readSize =
            ::GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb.data(), &dwSize,
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
      return ::DefWindowProc(hwnd, msg, wParam, lParam);
    }

    case WM_DPICHANGED:
      // unlike a Windows GUI app we don't want to resize the window if the DPI
      // changes as we want to respect the resolution selected by the user for
      // performance reasons but we do want to notify the module of the change
      // so it can adjust its rendering of text and other elements that are DPI
      // sensitive
    case WM_DISPLAYCHANGE: {
      PushRTMessage(DC_DISPLAY_CHANGE);
      return 0;
    }

    // WM_SIZE is sent when the user resizes the window.
    case WM_SIZE:
      if (wParam == SIZE_MINIMIZED) {
        _minimized.store(true);
      } else if (wParam == SIZE_MAXIMIZED) {
        _minimized.store(false);
        PushRTMessage(DC_WINDOW_MAXIMIZE, [lParam](auto &message) {
          message.Point = POINT{
              .x = LOWORD(lParam),
              .y = HIWORD(lParam),
          };
        });
      } else if (wParam == SIZE_RESTORED) {
        // Restored is any resize that is not a minimize or maximize.
        // For example, restoring the window to its default size
        // after a minimize or maximize, or from dragging the resize
        // bars.
        bool exp = true;
        if (_resizing.get()) {
          // Track, but don't resize until the user has finished resizing
          _resizing->x = LOWORD(lParam);
          _resizing->y = HIWORD(lParam);
        } else if (!_rtSwapChain ||
                   _minimized.compare_exchange_strong(exp, false)) {
          // If we are just starting up and haven't initialized d3d yet, or we
          // are simply restoring the window view without changing the size
        } else {
          PushRTMessage(DC_WINDOW_RESIZE, [lParam](auto &message) {
            message.Point = POINT{
                .x = LOWORD(lParam),
                .y = HIWORD(lParam),
            };
          });
        }
      }
      return 0;

    // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
    case WM_ENTERSIZEMOVE:
      _resizing = std::make_unique<POINT>();
      return 0;

    // Here we reset everything based on the new window dimensions.
    case WM_EXITSIZEMOVE:
      if (_resizing && _resizing->x && _resizing->y) {
        PushRTMessage(DC_WINDOW_RESIZE, [this](auto &message) {
          message.Point = *_resizing.get();
        });
      }
      _resizing.reset();
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
      if (!_rtCurrentFullScreen.FullScreen) {
        OnMoveWindow((int)(short)LOWORD(lParam) - _clientOffset.x,
                     (int)(short)HIWORD(lParam) - _clientOffset.y);
        PushRTMessage(DC_WINDOW_MOVE);
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
      ::PostQuitMessage(0);
      return 0;

    // Don't beep when we alt-enter.
    case WM_MENUCHAR:
      return MAKELRESULT(0, MNC_CLOSE);

    case WM_SETCURSOR: {
      if (LOWORD(lParam) == HTCLIENT) {
        if (OnHideCursor()) {
          return TRUE;
        }
      }
    }
      [[fallthrough]];

    default:
      return OnHandleMessage(hwnd, msg, wParam, lParam);
  }
}

}  // namespace core
}  // namespace MGDF
