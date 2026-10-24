#include "StdAfx.h"

#include "MGDFApp.hpp"

#include "MGDFD3D11RenderBackend.hpp"
#include "MGDFD3D12RenderBackend.hpp"
#include "common/MGDFPreferenceConstants.hpp"
#include "core.impl/MGDFGraphicsRequirements.hpp"
#include "core.impl/MGDFHostImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

MGDFApp::MGDFApp(ComObject<Host> &host, HINSTANCE hInstance)
    : D3DAppFramework(hInstance),
      _metrics(TIMER_SAMPLES),
      _host(host),
      _settings(host->GetRenderSettingsImpl()),
      _stInitialized(false),
      _rtFrameLimiter(nullptr),
      _stFrameLimiter(nullptr) {
  _ASSERTE(host);
  _host->InitGraphics();

  ::SecureZeroMemory(&_rtActiveEnd, sizeof(LARGE_INTEGER));
  ::SecureZeroMemory(&_rtStart, sizeof(LARGE_INTEGER));
  ::SecureZeroMemory(&_stEnd, sizeof(LARGE_INTEGER));

  host->GetGame(_game.Assign());
  host->GetTimer(_timer.Assign());
  host->GetDebugImpl()->SetMetrics(&_metrics);

  std::string pref;
  if (!GetPreference(_game, PreferenceConstants::SIM_FPS, pref)) {
    FATALERROR(_host,
               PreferenceConstants::SIM_FPS << " was not found in preferences");
  }

  _awaitFrame.test_and_set();

  const UINT32 simulationFps = FromString<UINT32>(pref);
  if (!simulationFps) {
    FATALERROR(_host, PreferenceConstants::SIM_FPS << " is not an integer");
  }

  if (FAILED(FrameLimiter::TryCreate(simulationFps, _stFrameLimiter))) {
    FATALERROR(_host, "Unable to create sim frame limiter");
  }

  if (GetPreference(_game, PreferenceConstants::RENDER_FPS, pref)) {
    if (FAILED(FrameLimiter::TryCreate(FromString<UINT32>(pref),
                                       _rtFrameLimiter))) {
      FATALERROR(_host, "Unable to create render frame limiter");
    }
  }

  _metrics.SetExpectedSimTime(1 / (double)simulationFps);

  _ASSERTE(host);
  _host = host;
  _host->SetShutDownHandler([this]() {
    _game->SavePreferences();
    CloseWindow();
  });
  _host->SetDeviceResetHandler([this]() { QueueResetDevice(); });
}

MGDFApp::~MGDFApp() {
  RTWaitForGpuIdle();
  _host->RTShutDown();
}

std::unique_ptr<IRenderBackend> MGDFApp::CreateRenderBackend(
    const ComObject<IDXGIFactory6> &factory) {
  if (_host->GetGraphicsAPI() == MGDF_GRAPHICS_API_D3D12) {
    return std::make_unique<D3D12RenderBackend>(
        factory, _host->GetGraphicsRequirements(),
        [this](const char *sender, const char *message) {
          FatalError(sender, message);
        });
  }
  return std::make_unique<D3D11RenderBackend>(
      factory,
      GetD3D11FeatureLevels(_host->GetGraphicsRequirements().MinFeatureLevel),
      [this](const char *sender, const char *message) {
        FatalError(sender, message);
      });
}

void MGDFApp::RTOnInitDevice(IRenderBackend &backend) {
  _host->RTSetDevices(backend);
}

bool MGDFApp::RTIsBackBufferChangePending() {
  return _settings->IsBackBufferChangePending();
}

bool MGDFApp::RTVSyncEnabled() const { return _settings->GetVSync(); }

MGDFFullScreenDesc MGDFApp::RTOnResetSwapChain(
    DXGI_SWAP_CHAIN_DESC1 &swapDesc,
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreenDesc, const RECT &windowSize) {
  _settings->OnResetSwapChain(swapDesc, fullscreenDesc, windowSize);
  MGDFFullScreenDesc desc;
  _settings->GetFullscreen(&desc);
  return desc;
}

std::optional<UINT> MGDFApp::RTGetMaxFrameLatency() const {
  MGDFFullScreenDesc desc;
  _settings->GetFullscreen(&desc);
  if (!desc.ExclusiveMode) {
    return _settings->GetMaxFrameLatency();
  }
  return std::nullopt;
}

void MGDFApp::RTOnResize(UINT32 width, UINT32 height) {
  _settings->OnResize(width, height);
}

bool MGDFApp::OnInitWindow(RECT &window) {
  std::string pref;
  window.top = GetPreference(_game, PreferenceConstants::WINDOW_POSITIONY, pref)
                   ? FromString<LONG>(pref)
                   : 0;
  window.left =
      GetPreference(_game, PreferenceConstants::WINDOW_POSITIONX, pref)
          ? FromString<LONG>(pref)
          : 0;
  window.right = window.left +
                 (GetPreference(_game, PreferenceConstants::WINDOW_SIZEX, pref)
                      ? FromString<LONG>(pref)
                      : 0);
  window.bottom = window.top +
                  (GetPreference(_game, PreferenceConstants::WINDOW_SIZEY, pref)
                       ? FromString<LONG>(pref)
                       : 0);
  return GetPreference(_game, PreferenceConstants::WINDOW_RESIZE, pref) &&
         FromString<int>(pref) == 1;
}

void MGDFApp::RTOnBeforeDeviceReset() {
  RTOnBeforeBackBufferChange();
  _host->RTBeforeDeviceReset();
}

void MGDFApp::RTOnDeviceReset() { _host->RTDeviceReset(); }

void MGDFApp::RTOnBeforeBackBufferChange() {
  _host->RTBeforeBackBufferChange();
}

void MGDFApp::RTOnBackBufferChange(IRenderBackend &backend) {
  _host->RTBackBufferChange(backend);
}

void MGDFApp::RTOnBeforeFirstDraw() {
  _rtStart = _rtActiveEnd = _timer->GetCurrentTimeTicks();

  // wait for one sim frame to finish before
  // we start drawing
  while (_awaitFrame.test_and_set()) {
    ::Sleep(1);
  }
  _host->RTBeforeFirstDraw();
}

bool MGDFApp::RTOnDraw() {
  bool didLimit = false;
  const LARGE_INTEGER currentTime = _rtFrameLimiter
                                        ? _rtFrameLimiter->LimitFps(didLimit)
                                        : _timer->GetCurrentTimeTicks();

  const double elapsedTime =
      _timer->ConvertDifferenceToSeconds(currentTime, _rtStart);
  _metrics.AppendRenderTimes(
      elapsedTime, _timer->ConvertDifferenceToSeconds(_rtActiveEnd, _rtStart));
  _rtStart = currentTime;

  const bool drawn = _host->RTDraw(elapsedTime);

  _rtActiveEnd = _timer->GetCurrentTimeTicks();
  return drawn;
}

void MGDFApp::RTOnAfterPresent() { _host->RTAfterPresent(); }

std::pair<DXGI_FORMAT, DXGI_FORMAT> MGDFApp::RTOnBeforeEnumerateDisplayModes() {
  return std::make_pair(_settings->GetSDRBackBufferFormat(),
                        _settings->GetHDRBackBufferFormat());
}

void MGDFApp::RTOnDisplayChange(
    const DXGI_OUTPUT_DESC1 &currentOutputDesc, UINT currentDPI,
    ULONG currentSDRWhiteLevel,
    const std::vector<DXGI_MODE_DESC1> &primaryOutputModes) {
  const UINT32 nativeWidth = static_cast<UINT32>(GetSystemMetrics(SM_CXSCREEN));
  const UINT32 nativeHeight =
      static_cast<UINT32>(GetSystemMetrics(SM_CYSCREEN));

  std::map<std::tuple<UINT32, UINT32, UINT, UINT>, MGDFDisplayMode> uniqueModes;

  for (const auto &mode : primaryOutputModes) {
    const auto key =
        std::make_tuple(mode.Width, mode.Height, mode.RefreshRate.Numerator,
                        mode.RefreshRate.Denominator);

    const bool isHDR = mode.Format == _settings->GetHDRBackBufferFormat();

    const auto &found = uniqueModes.find(key);
    if (found == uniqueModes.end()) {
      uniqueModes.insert(std::make_pair(
          key, MGDFDisplayMode{
                   .Width = mode.Width,
                   .Height = mode.Height,
                   .RefreshRateNumerator = mode.RefreshRate.Numerator,
                   .RefreshRateDenominator = mode.RefreshRate.Denominator,
                   .SupportsHDR = isHDR,
                   .IsNativeSize = mode.Width == nativeWidth &&
                                   mode.Height == nativeHeight}));
    } else if (isHDR) {
      found->second.SupportsHDR = true;
    }
  }
  std::vector<MGDFDisplayMode> modes;
  modes.reserve(uniqueModes.size());
  for (const auto &pair : uniqueModes) {
    modes.push_back(pair.second);
  }
  if (modes.size() == 0) {
    FATALERROR(this, "No display modes found");
  }

  const bool supportsHDR = currentOutputDesc.ColorSpace ==
                           DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
  const MGDFOutputDisplayInfo info{
      .SupportsHDR = supportsHDR,
      .MaxFullFrameLuminance = currentOutputDesc.MaxFullFrameLuminance,
      .MaxLuminance = currentOutputDesc.MaxLuminance,
      .MinLuminance = currentOutputDesc.MinLuminance,
      .SDRWhiteLevel = supportsHDR ? currentSDRWhiteLevel : 1000,
      .Width = static_cast<UINT32>(currentOutputDesc.DesktopCoordinates.right -
                                   currentOutputDesc.DesktopCoordinates.left),
      .Height =
          static_cast<UINT32>(currentOutputDesc.DesktopCoordinates.bottom -
                              currentOutputDesc.DesktopCoordinates.top),
  };

  _host->GetRenderSettingsImpl()->SetOutputProperties(info, currentDPI, modes);
}

void MGDFApp::STOnUpdateSim() {
  if (!_stInitialized) {
    _stEnd = _timer->GetCurrentTimeTicks();
    LOG("Creating Module...", MGDF_LOG_LOW);
    _host->STCreateModule();
    _stInitialized = true;
  }
  const LARGE_INTEGER simulationEnd = _stEnd;

  // execute one frame of game logic as per the current module
  _host->STUpdate(_metrics.ExpectedSimTime(), _metrics);
  _awaitFrame.clear();

  const LARGE_INTEGER activeSimulationEnd = _timer->GetCurrentTimeTicks();
  _metrics.AppendActiveSimTime(
      _timer->ConvertDifferenceToSeconds(activeSimulationEnd, simulationEnd));

  // wait until the next frame to begin if we have any spare time left over
  bool didLimit;
  _stEnd = _stFrameLimiter->LimitFps(didLimit);
  _metrics.AppendSimTime(
      _timer->ConvertDifferenceToSeconds(_stEnd, simulationEnd));
}

void MGDFApp::OnRawInput(std::function<RAWINPUT *()> getInput) {
  _host->GetInputManagerImpl()->HandleInput(getInput);
}

void MGDFApp::OnMouseInput(INT32 x, INT32 y) {
  _host->GetInputManagerImpl()->HandleInput(x, y);
}

void MGDFApp::OnExternalClose() { _host->QueueShutDown(); }

void MGDFApp::OnMoveWindow(INT32 x, INT32 y) {
  std::stringstream xs;
  xs << x;
  _game->SetPreference(PreferenceConstants::WINDOW_POSITIONX, xs.str().c_str());
  std::stringstream ys;
  ys << y;
  _game->SetPreference(PreferenceConstants::WINDOW_POSITIONY, ys.str().c_str());
}

bool MGDFApp::OnSetCursor() {
  const auto input = _host->GetInputManagerImpl();
  if (!input->GetShowCursor()) {
    ::SetCursor(NULL);
    return true;
  }
  LPCTSTR shape = nullptr;
  switch (input->GetCursorShape()) {
    case MGDF_CURSOR_HAND:
      shape = IDC_HAND;
      break;
    case MGDF_CURSOR_SIZE_NWSE:
      shape = IDC_SIZENWSE;
      break;
    case MGDF_CURSOR_SIZE_NS:
      shape = IDC_SIZENS;
      break;
    case MGDF_CURSOR_SIZE_WE:
      shape = IDC_SIZEWE;
      break;
    case MGDF_CURSOR_SIZE_ALL:
      shape = IDC_SIZEALL;
      break;
    default:
      return false;
  }
  ::SetCursor(::LoadCursor(nullptr, shape));
  return true;
}

LRESULT MGDFApp::OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                                 LPARAM lParam) {
  switch (msg) {
    case WM_SYSKEYDOWN:
      return 0;
    case WM_ACTIVATE:
      if (wParam == WA_ACTIVE || wParam == WA_CLICKACTIVE) {
        _host->GetInputManagerImpl()->ClearInput();
      }
      [[fallthrough]];
    default:
      return ::DefWindowProc(hwnd, msg, wParam, lParam);
  }
}

void MGDFApp::FatalError(const char *sender, const char *message) {
  _ASSERTE(sender);
  _ASSERTE(message);
  _host->FatalError(sender, message);
}

}  // namespace core
}  // namespace MGDF