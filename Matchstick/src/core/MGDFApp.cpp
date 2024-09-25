#include "StdAfx.h"

#include "MGDFApp.hpp"

#include "common/MGDFPreferenceConstants.hpp"
#include "core.impl/MGDFHostImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

MGDFApp::MGDFApp(Host *host, HINSTANCE hInstance)
    : D3DAppFramework(hInstance),
      _stats(TIMER_SAMPLES),
      _host(host),
      _settings(host->GetRenderSettingsImpl()),
      _stInitialized(false),
      _rtFrameLimiter(nullptr),
      _stFrameLimiter(nullptr) {
  _ASSERTE(host);

  ::SecureZeroMemory(&_rtActiveEnd, sizeof(LARGE_INTEGER));
  ::SecureZeroMemory(&_rtStart, sizeof(LARGE_INTEGER));
  ::SecureZeroMemory(&_rtTextMetrics, sizeof(DWRITE_TEXT_METRICS));
  ::SecureZeroMemory(&_stEnd, sizeof(LARGE_INTEGER));

  host->GetGame(_game.Assign());
  host->GetTimer(_timer.Assign());

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

  _stats.SetExpectedSimTime(1 / (double)simulationFps);

  _ASSERTE(host);
  _host = host;
  _host->SetShutDownHandler([this]() {
    _game->SavePreferences();
    CloseWindow();
  });
  _host->SetDeviceResetHandler([this]() { QueueResetDevice(); });

  RTInitDirectWrite();
}

MGDFApp::~MGDFApp() {}

UINT64 MGDFApp::GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                              UINT64 *featureLevelsSize) {
  return _host->GetCompatibleD3DFeatureLevels(levels, featureLevelsSize);
}

void MGDFApp::RTOnInitDevices(const ComObject<ID3D11Device> &d3dDevice,
                              const ComObject<ID2D1Device> &d2dDevice) {
  _ASSERTE(d3dDevice);
  _ASSERTE(d2dDevice);

  if (FAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                            _rtContext.Assign()))) {
    FATALERROR(this, "Unable to create ID2D1DeviceContext");
  }

  _host->RTSetDevices(d3dDevice, d2dDevice);
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

void MGDFApp::RTOnSwapChainCreated(ComObject<IDXGISwapChain1> &swapchain) {
  MGDFFullScreenDesc desc;
  _settings->GetFullscreen(&desc);
  if (!desc.ExclusiveMode) {
    swapchain.As<IDXGISwapChain2>()->SetMaximumFrameLatency(
        _settings->GetMaxFrameLatency());
  }
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
  _rtContext.Clear();
  _rtBlackBrush.Clear();
  _rtWhiteBrush.Clear();
  _rtTextLayout.Clear();
  _rtTextStream.reset();
  _rtTextFormat.Clear();
  _rtDWriteFactory.Clear();
  _host->RTBeforeDeviceReset();
}

void MGDFApp::RTOnDeviceReset() {
  RTInitDirectWrite();
  _host->RTDeviceReset();
}

void MGDFApp::RTInitDirectWrite() {
  if (FAILED(::DWriteCreateFactory(
          DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory1),
          reinterpret_cast<IUnknown **>(_rtDWriteFactory.Assign())))) {
    FATALERROR(_host, "Unable to create IDWriteFactory");
  }

  ComObject<IDWriteFontCollection> fontCollection;
  if (FAILED(
          _rtDWriteFactory->GetSystemFontCollection(fontCollection.Assign()))) {
    FATALERROR(_host, "Unable to get  font collection");
  }

  if (FAILED(_rtDWriteFactory->CreateTextFormat(
          L"Arial", fontCollection, DWRITE_FONT_WEIGHT_NORMAL,
          DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14, L"",
          _rtTextFormat.Assign()))) {
    FATALERROR(_host, "Unable to create text format");
  }

  _rtTextStream = std::make_unique<TextStream>(_rtDWriteFactory);
}

void MGDFApp::RTOnBeforeBackBufferChange() {
  _rtContext->SetTarget(nullptr);
  _host->RTBeforeBackBufferChange();
}

void MGDFApp::RTOnBackBufferChange(
    const ComObject<ID3D11Texture2D> &backBuffer,
    const ComObject<ID3D11Texture2D> &depthStencilBuffer) {
  _ASSERTE(backBuffer);
  _ASSERTE(depthStencilBuffer);

  _host->RTBackBufferChange(backBuffer, depthStencilBuffer);
  _host->SetBackBufferRenderTarget(_rtContext);
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

void MGDFApp::RTOnDraw() {
  bool didLimit = false;
  const LARGE_INTEGER currentTime = _rtFrameLimiter
                                        ? _rtFrameLimiter->LimitFps(didLimit)
                                        : _timer->GetCurrentTimeTicks();

  const double elapsedTime =
      _timer->ConvertDifferenceToSeconds(currentTime, _rtStart);
  _stats.AppendRenderTimes(
      elapsedTime, _timer->ConvertDifferenceToSeconds(_rtActiveEnd, _rtStart));
  _rtStart = currentTime;

  _host->RTDraw(elapsedTime);

  if (_host->GetDebugImpl()->IsShown()) {
    RTDrawSystemOverlay();
  }
  _rtActiveEnd = _timer->GetCurrentTimeTicks();
}

void MGDFApp::RTDrawSystemOverlay() {
  if (!_rtWhiteBrush || !_rtBlackBrush) {
    RTInitBrushes();
  }

  _rtTextStream->ClearText();
  _host->GetDebugImpl()->DumpInfo(_stats, *_rtTextStream);

  if (FAILED(_rtTextStream->GenerateLayout(
          _rtContext, _rtTextFormat,
          static_cast<float>(_settings->GetScreenX()),
          static_cast<float>(_settings->GetScreenY()), _rtTextLayout))) {
    FATALERROR(_host, "Unable to create text layout");
  }

  ::SecureZeroMemory(&_rtTextMetrics, sizeof(_rtTextMetrics));
  if (FAILED(_rtTextLayout->GetMetrics(&_rtTextMetrics))) {
    FATALERROR(_host, "Unable to get text overhang metrics");
  }

  _rtContext->BeginDraw();

  constexpr float margin = 5.0f;
  const D2D1_ROUNDED_RECT rect{
      .rect = {.left = margin,
               .top = margin,
               .right = (margin * 3) + _rtTextMetrics.width,
               .bottom = (margin * 3) + _rtTextMetrics.height},
      .radiusX = margin,
      .radiusY = margin,
  };
  _ASSERTE(_rtBlackBrush);
  _rtContext->FillRoundedRectangle(&rect, _rtBlackBrush);
  _ASSERTE(_rtWhiteBrush);
  _rtContext->DrawRoundedRectangle(&rect, _rtWhiteBrush);

  const D2D_POINT_2F origin{
      .x = 2 * margin,
      .y = 2 * margin,
  };
  _rtContext->DrawTextLayout(origin, _rtTextLayout, _rtWhiteBrush);

  _rtContext->EndDraw();
}

void MGDFApp::RTInitBrushes() {
  D2D1_COLOR_F color{.r = 1.0f, .g = 1.0f, .b = 1.0f, .a = 1.0f};
  if (FAILED(
          _rtContext->CreateSolidColorBrush(color, _rtWhiteBrush.Assign()))) {
    FATALERROR(_host, "Unable to create white color brush");
  }

  color.r = color.g = color.b = 0.05f;
  color.a = 0.85f;
  if (FAILED(
          _rtContext->CreateSolidColorBrush(color, _rtBlackBrush.Assign()))) {
    FATALERROR(_host, "Unable to create black color brush");
  }
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

    const bool isHDR = mode.Format == DXGI_FORMAT_R16G16B16A16_FLOAT;

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

  const bool hdrSupported = currentOutputDesc.ColorSpace ==
                            DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
  const MGDFHDRDisplayInfo info{
      .Supported = hdrSupported,
      .MaxFullFrameLuminance = currentOutputDesc.MaxFullFrameLuminance,
      .MaxLuminance = currentOutputDesc.MaxLuminance,
      .MinLuminance = currentOutputDesc.MinLuminance,
      .SDRWhiteLevel = hdrSupported ? currentSDRWhiteLevel : 1000,
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
  _host->STUpdate(_stats.ExpectedSimTime(), _stats);
  _awaitFrame.clear();

  const LARGE_INTEGER activeSimulationEnd = _timer->GetCurrentTimeTicks();
  _stats.AppendActiveSimTime(
      _timer->ConvertDifferenceToSeconds(activeSimulationEnd, simulationEnd));

  // wait until the next frame to begin if we have any spare time left over
  bool didLimit;
  _stEnd = _stFrameLimiter->LimitFps(didLimit);
  _stats.AppendSimTime(
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

bool MGDFApp::OnHideCursor() {
  if (!_host->GetInputManagerImpl()->GetShowCursor()) {
    ::SetCursor(NULL);
    return true;
  }
  return false;
}

LRESULT MGDFApp::OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                                 LPARAM lParam) {
  switch (msg) {
    case WM_SYSKEYDOWN:
      switch (wParam) {
        case VK_F12:
          _host->GetDebugImpl()->ToggleShown();
          return 0;
        default:
          return 0;
      }
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