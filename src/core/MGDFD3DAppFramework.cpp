#include "StdAfx.h"

#include "MGDFD3DAppFramework.hpp"

#include <optional>
#include <string>

#include "common/MGDFLoggerImpl.hpp"
#include "common/MGDFParameterManager.hpp"
#include "common/MGDFResources.hpp"
#include "core.impl/MGDFParameterConstants.hpp"
#include "windowsx.h"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

typedef uint64_t QWORD;

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

constexpr UINT MAX_RAWINPUT_BUFFER_SIZE = 1024 * 1024;
constexpr UINT RAWINPUT_QUEUE_SIZE = 1024;

static std::unordered_map<HWND, D3DAppFramework *> windowMappings;

static LRESULT CALLBACK StaticWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                         LPARAM lParam) {
  if (msg == WM_NCCREATE) {
    CREATESTRUCT *create = (CREATESTRUCT *)lParam;
    windowMappings.insert(std::make_pair(
        hwnd, static_cast<D3DAppFramework *>(create->lpCreateParams)));
  }

  const auto it = windowMappings.find(hwnd);
  if (it != windowMappings.end()) {
    return it->second->MsgProc(hwnd, msg, wParam, lParam);
  }

  if (msg == WM_NCDESTROY) {
    windowMappings.erase(hwnd);
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

#define WINDOW_CLASS_NAME "MGDFD3DAppFrameworkWindowClass"

D3DAppFramework::D3DAppFramework(HINSTANCE hInstance)
    : _applicationInstance(hInstance),
      _window(nullptr),
      _renderThread(nullptr),
      _internalShutDown(false),
      _windowStyle(WS_OVERLAPPEDWINDOW),
      _rtAllowTearing(false),
      _hasSwapChain(false) {
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

void D3DAppFramework::InitWindow(const std::string &caption) {
  // if the window has not already been created
  if (!_window) {
    LOG("Initializing window...", MGDF_LOG_LOW);
    const WNDCLASS wc{
        .style = CS_HREDRAW | CS_VREDRAW,
        .lpfnWndProc = StaticWindowProc,
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
                             _applicationInstance, this);

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

    _rtRenderBackend = CreateRenderBackend(_rtFactory);

    if (!RTInitD3D(_window)) {
      FATALERROR(this, "Failed to initialize D3D");
    }
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
          .usUsagePage = 0x01,  // desktop input
          .usUsage = 0x06,      // keyboard
          .dwFlags =
              0,  // Allow application shortcuts while the game has focus.
          .hwndTarget = _window,
      }};

  if (!::RegisterRawInputDevices(Rid, 2, sizeof(Rid[0]))) {
    FATALERROR(this,
               "Failed to register raw input devices for mouse and keyboard");
  }
}

bool D3DAppFramework::RTInitD3D(const HWND window) {
  LOG("Initializing Direct3D...", MGDF_LOG_LOW);

  _rtFactory = RTCreateDXGIFactory();

  BOOL allowTearing = FALSE;
  if (!_rtFactory || FAILED(_rtFactory->CheckFeatureSupport(
                         DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing,
                         sizeof(allowTearing)))) {
    allowTearing = FALSE;
  }
  _rtAllowTearing = allowTearing == TRUE;

  if (!_rtRenderBackend->RTInit()) {
    return false;
  }
  _rtAdapter = _rtRenderBackend->RTGetAdapter();

  if (!RTCheckForDisplayChanges(window)) {
    return false;
  }
  RTOnInitDevice(*_rtRenderBackend);

  RECT windowSize;
  if (!::GetClientRect(window, &windowSize)) {
    FATALERROR(this, "GetClientRect failed");
  }
  RTWaitForGpuIdle();
  RTOnBeforeBackBufferChange();
  _rtCurrentFullScreen =
      RTOnResetSwapChain(_rtSwapDesc, _rtFullscreenSwapDesc, windowSize);
  // the window starts windowed: restyle it before the swapchain is created,
  // or a fullscreen sized swapchain presents into the small window
  if (_rtCurrentFullScreen.FullScreen && !_rtCurrentFullScreen.ExclusiveMode) {
    ApplyWindowMode(window, true);
  }
  RTCreateSwapChain(window);
  if (_rtCurrentFullScreen.FullScreen && _rtCurrentFullScreen.ExclusiveMode) {
    _rtRenderBackend->RTSetExclusiveFullscreen();
  }
  RTResizeBackBuffer();
  return true;
}

void D3DAppFramework::ApplyWindowMode(const HWND window,
                                      const bool fullScreenBorderless) {
  if (fullScreenBorderless) {
    LOG("Setting fullscreen-borderless mode", MGDF_LOG_LOW);
    if (!::SetWindowLongW(
            window, GWL_STYLE,
            WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                                    WS_MINIMIZEBOX | WS_MAXIMIZEBOX))) {
      FATALERROR(this, "SetWindowLongW failed");
    }
    HMONITOR hMonitor = ::MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX monitorInfo = {};
    monitorInfo.cbSize = sizeof(MONITORINFOEX);
    if (!::GetMonitorInfo(hMonitor, &monitorInfo)) {
      FATALERROR(this, "GetMonitorInfo failed");
    }
    if (!::SetWindowPos(
            window, HWND_TOP, monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top,
            monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE)) {
      FATALERROR(this, "SetWindowPos failed");
    }
    ::ShowWindow(window, SW_MAXIMIZE);
  } else {
    LOG("Setting windowed mode", MGDF_LOG_LOW);
    if (!::SetWindowLong(window, GWL_STYLE, _windowStyle)) {
      FATALERROR(this, "SetWindowLong failed");
    }
    if (!::SetWindowPos(window, HWND_NOTOPMOST, _rtWindowRect.left,
                        _rtWindowRect.top,
                        _rtWindowRect.right - _rtWindowRect.left,
                        _rtWindowRect.bottom - _rtWindowRect.top,
                        SWP_FRAMECHANGED | SWP_NOACTIVATE)) {
      FATALERROR(this, "SetWindowPos failed");
    }
    ::ShowWindow(window, SW_NORMAL);
  }
}

ComObject<IDXGIFactory6> D3DAppFramework::RTCreateDXGIFactory() {
#if defined(DEBUG) || defined(_DEBUG)
  constexpr auto flags = DXGI_CREATE_FACTORY_DEBUG;
#else
  constexpr auto flags = 0U;
#endif
  ComObject<IDXGIFactory6> factory;
  // Visual studio graphics debugger doesn't like it when you create a
  // DXGIFactory2 sometimes - no idea why, but to get it working, you can pass
  // -usedefaultadapter to skip adapter selection and use the default adapter.
  if (!ParameterManager::Instance().HasParameter(
          ParameterConstants::USE_DEFAULT_ADAPTER)) {
    if (FAILED(::CreateDXGIFactory2(flags, IID_PPV_ARGS(factory.Assign())))) {
      FATALERROR(this, "Failed to create IDXGIFactory6");
    }
  }
  return factory;
}

void D3DAppFramework::RTPrepareToReinitD3D() {
  const HRESULT reason = _rtRenderBackend->RTGetDeviceRemovedReason();
  LOG("Device removed! DXGI_ERROR code " << reason, MGDF_LOG_ERROR);

  _awaitingD3DReset.store(true);
  RTWaitForGpuIdle();
  RTOnBeforeDeviceReset();
  RTUninitD3D();
}

void D3DAppFramework::RTUninitD3D() {
  LOG("Cleaning up Direct3D resources...", MGDF_LOG_LOW);
  RTWaitForGpuIdle();
  _rtAdapter.Clear();
  if (_rtRenderBackend) {
    _rtRenderBackend->RTUninit(_rtCurrentFullScreen.ExclusiveMode);
  }
  _hasSwapChain.store(false);
  _rtFactory.Clear();
}

void D3DAppFramework::RTWaitForGpuIdle() {
  if (_rtRenderBackend) {
    _rtRenderBackend->RTWaitForGpuIdle();
  }
}

bool D3DAppFramework::RTCheckForDisplayChanges(const HWND window) {
  UINT i = 0;
  ComObject<IDXGIOutput> currentOutput;
  ComObject<IDXGIOutput6> bestOutput;
  ComObject<IDXGIOutput6> primaryOutput;
  float bestIntersectArea = -1;

  const auto displayModeFormats = RTOnBeforeEnumerateDisplayModes();

  // the best matching output is the one with the largest intersection area
  // with the app window
  LOG("Checking outputs to find best match for current window...",
      MGDF_LOG_HIGH);
  while (_rtAdapter->EnumOutputs(i, currentOutput.Assign()) !=
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
    LOG("No outputs found", MGDF_LOG_ERROR);
    return false;
  }

  DXGI_OUTPUT_DESC1 primaryDesc;
  if (FAILED(primaryOutput->GetDesc1(&primaryDesc))) {
    FATALERROR(
        this,
        "Failed to get description from primary output overlapping window");
  }

  UINT32 maxSDRAdaptorModes = 0U;
  primaryOutput->GetDisplayModeList1(displayModeFormats.first, 0,
                                     &maxSDRAdaptorModes, nullptr);

  UINT32 maxHDRAdaptorModes = 0U;
  if (primaryDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
    LOG("Primary output supports HDR", MGDF_LOG_LOW);
    primaryOutput->GetDisplayModeList1(displayModeFormats.second, 0,
                                       &maxHDRAdaptorModes, nullptr);
  }

  std::vector<DXGI_MODE_DESC1> primaryModes(
      static_cast<size_t>(maxSDRAdaptorModes) +
      static_cast<size_t>(maxHDRAdaptorModes));

  // get all valid modes for the outputs supported
  if (FAILED(primaryOutput->GetDisplayModeList1(displayModeFormats.first, 0,
                                                &maxSDRAdaptorModes,
                                                primaryModes.data())) ||
      (maxHDRAdaptorModes > 0U &&
       FAILED(primaryOutput->GetDisplayModeList1(
           displayModeFormats.second, 0, &maxHDRAdaptorModes,
           primaryModes.data() + maxSDRAdaptorModes)))) {
    LOG("Failed to get mode lists from adapter", MGDF_LOG_ERROR);
    return false;
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
        DISPLAYCONFIG_SOURCE_DEVICE_NAME deviceName = {
            .header = {.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,
                       .size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME),
                       .adapterId = path.sourceInfo.adapterId,
                       .id = path.sourceInfo.id}};
        // check if this path matches the output we are interested in
        if ((::DisplayConfigGetDeviceInfo(&deviceName.header) ==
             ERROR_SUCCESS) &&
            (::wcscmp(monitorInfo.szDevice, deviceName.viewGdiDeviceName) ==
             0)) {
          // query the reference SDR white level
          DISPLAYCONFIG_SDR_WHITE_LEVEL whiteLevel = {
              .header = {.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL,
                         .size = sizeof(DISPLAYCONFIG_SDR_WHITE_LEVEL),
                         .adapterId = path.targetInfo.adapterId,
                         .id = path.targetInfo.id}};
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
  return true;
}

void D3DAppFramework::RTReinitD3D(const HWND window) {
  constexpr int maxRetries = 5;
  constexpr int retryDelayMs = 1000;
  for (int attempt = 0; attempt <= maxRetries; ++attempt) {
    if (attempt > 0) {
      LOG("D3D reinit attempt " << (attempt + 1) << " of " << (maxRetries + 1)
                                << "...",
          MGDF_LOG_LOW);
      RTUninitD3D();
      ::Sleep(retryDelayMs);
    }
    if (RTInitD3D(window)) {
      RTOnDeviceReset();
      return;
    }
  }
  FATALERROR(this, "Failed to reinitialize D3D after multiple attempts");
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

  RTWaitForGpuIdle();
  _rtRenderBackend->RTCreateSwapChain(window, _rtSwapDesc,
                                      RTGetMaxFrameLatency());
  _hasSwapChain.store(true);
}

void D3DAppFramework::RTResizeBackBuffer() {
  RTWaitForGpuIdle();
  const HRESULT result = _rtRenderBackend->RTResizeBackBuffer(_rtSwapDesc);

  // Resize the swap chain and recreate the render target view.
  if (result == DXGI_ERROR_DEVICE_REMOVED ||
      result == DXGI_ERROR_DEVICE_RESET) {
    RTPrepareToReinitD3D();
    return;
  } else if (FAILED(result)) {
    FATALERROR(this, "Failed to resize swapchain buffers");
  }

  RTOnBackBufferChange(*_rtRenderBackend);
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
    std::optional<DisplayChangeMessage> &message) {
  std::lock_guard<std::mutex> lock(_displayChangeMutex);
  if (!_pendingDisplayChanges.empty()) {
    message = _pendingDisplayChanges.front();
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
  if (!_window && (!_rtRenderBackend || !_rtRenderBackend->RTIsInitialized())) {
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
      [this](const HWND window) {
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

          if (!_rtRenderBackend->RTIsInitialized()) {
            if (!awaitingReset) {
              LOG("Reinitializing D3D after Device Reset...", MGDF_LOG_MEDIUM);
              RTReinitD3D(window);
            } else {
              continue;
            }
          } else {
            const auto dxgiFactoryIsCurrent =
                !_rtFactory || _rtFactory->IsCurrent();
            if (!dxgiFactoryIsCurrent) {
              LOG("DXGI factory is no longer current, recreating...",
                  MGDF_LOG_LOW);
              _rtFactory = RTCreateDXGIFactory();
            }

            // get the most recent display change message (if any)
            std::optional<DisplayChangeMessage> displayChange;
            if (PopRTMessage(displayChange) || !dxgiFactoryIsCurrent) {
              if (!RTCheckForDisplayChanges(window)) {
                LOG("Failed to check for display changes, will retry later",
                    MGDF_LOG_LOW);
              }
            }

            // for window moves and resizes that don't result
            // in a fullscreen presentation, record the window
            // size so that restore to/from fullscreen works and
            // the window size is recorded for next startup
            if (displayChange &&
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
              RTWaitForGpuIdle();
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

              RTWaitForGpuIdle();
              RTOnBeforeBackBufferChange();
              const MGDFFullScreenDesc newFullScreen = RTOnResetSwapChain(
                  _rtSwapDesc, _rtFullscreenSwapDesc, windowSize);

              if (_rtCurrentFullScreen.ExclusiveMode) {
                _rtRenderBackend->RTSetWindowed();
              }
              _rtCurrentFullScreen = newFullScreen;

              if (!newFullScreen.ExclusiveMode) {
                ApplyWindowMode(window, newFullScreen.FullScreen);
              }
              RTCreateSwapChain(window);

              // reset the swap chain to fullscreen if it was previously
              // fullscreen or if this backbuffer change was for a toggle from
              // windowed to fullscreen
              if (newFullScreen.FullScreen && newFullScreen.ExclusiveMode) {
                _rtRenderBackend->RTSetExclusiveFullscreen();
              }
              RTResizeBackBuffer();
            }
          }

          if (!_minimized.load() && _rtRenderBackend->RTIsInitialized()) {
            _rtRenderBackend->RTWaitForFrame();
            _rtRenderBackend->RTClear();

            RTOnDraw();

            const HRESULT result = _rtRenderBackend->RTPresent(
                RTVSyncEnabled() ? 1 : 0,
                RTAllowTearing() ? DXGI_PRESENT_ALLOW_TEARING : 0);
            RTOnAfterPresent();

            if (result == DXGI_ERROR_DEVICE_REMOVED ||
                result == DXGI_ERROR_DEVICE_RESET) {
              RTPrepareToReinitD3D();
            } else if (FAILED(result)) {
              FATALERROR(this, "Direct3d Present1 failed");
            }
          }
        }
        LOG("Stopping render thread...", MGDF_LOG_LOW);
      },
      _window);

  MSG msg{
      .message = WM_NULL,
  };
  LOG("Starting input loop...", MGDF_LOG_LOW);

  // deal with any windows messages on the main thread, this allows us
  // to ensure that any user input is handled with as little latency as
  // possible independent of the update rate for the sim and render threads.
  bool runMainThread = true;
  while (runMainThread) {
    Sleep(1);
    ProcessRawInput();
    const bool hasFocus = GetForegroundWindow() == _window;
    while (
        !hasFocus
            // process all messages to ensure waking up reliably
            ? ::PeekMessage(&msg, _window, 0, 0, PM_REMOVE)
            // only process non WM_INPUT messages as WM_INPUT is handled
            // above by GetRawInputBuffer
            : (::PeekMessage(&msg, _window, 0, WM_INPUT - 1, PM_REMOVE) != 0 ||
               ::PeekMessage(&msg, _window, WM_INPUT + 1, (UINT)-1,
                             PM_REMOVE) != 0)) {
      if (msg.message == WM_QUIT) {
        runMainThread = false;
        break;
      }
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }

  LOG("Stopping input loop...", MGDF_LOG_LOW);

  runSimThread.clear();
  simThread.join();

  return (int)msg.wParam;
}

void D3DAppFramework::ProcessRawInput() {
  UINT minRawInputBufferSize = 0U;
  if (GetRawInputBuffer(NULL, &minRawInputBufferSize, sizeof(RAWINPUTHEADER)) !=
      0) {
    FATALERROR(this,
               "GetRawInputBuffer failed. Error code: " << GetLastError());
  }
  const UINT rawInputBufferSize = std::min<UINT>(
      minRawInputBufferSize * RAWINPUT_QUEUE_SIZE, MAX_RAWINPUT_BUFFER_SIZE);
  if (_rawInputBuffer.size() < rawInputBufferSize) {
    _rawInputBuffer.resize(rawInputBufferSize);
  }

  UINT numEvents = 0U;
  do {
    UINT currentRawInputBufferSize = static_cast<UINT>(_rawInputBuffer.size());
    numEvents =
        GetRawInputBuffer((RAWINPUT *)_rawInputBuffer.data(),
                          &currentRawInputBufferSize, sizeof(RAWINPUTHEADER));
    if (numEvents == -1) {
      FATALERROR(this,
                 "GetRawInputBuffer failed. Error code: " << GetLastError());
    }

    RAWINPUT *currentRawInput = (RAWINPUT *)_rawInputBuffer.data();
    UINT i = 0;
    OnRawInput([&i, &currentRawInput, numEvents]() -> RAWINPUT * {
      if (i++ >= numEvents) {
        return nullptr;
      }
      RAWINPUT *current = currentRawInput;
      currentRawInput = NEXTRAWINPUTBLOCK(currentRawInput);
      return current;
    });
  } while (numEvents > 0);
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

    case WM_DPICHANGED:
      // unlike a Windows GUI app we don't want to resize the window if the
      // DPI changes as we want to respect the resolution selected by the user
      // for performance reasons but we do want to notify the module of the
      // change so it can adjust its rendering of text and other elements that
      // are DPI sensitive
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
        } else if (!_hasSwapChain.load() ||
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
      // NULL out window so that the message loop can find the WM_QUIT message
      // as it won't be associated with a window
      _window = NULL;
      ::PostQuitMessage(0);
      return 0;

    // Don't beep when we alt-enter.
    case WM_MENUCHAR:
      return MAKELRESULT(0, MNC_CLOSE);

    case WM_SETCURSOR: {
      if (LOWORD(lParam) == HTCLIENT) {
        if (OnSetCursor()) {
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
