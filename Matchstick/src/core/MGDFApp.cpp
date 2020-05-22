#include "StdAfx.h"

#include "MGDFApp.hpp"

#include "common/MGDFPreferenceConstants.hpp"
#include "core.impl/MGDFHostImpl.hpp"
#include "core.impl/MGDFParameterConstants.hpp"

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
      _blackBrush(nullptr),
      _whiteBrush(nullptr),
      _textFormat(nullptr),
      _dWriteFactory(nullptr),
      _initialized(false),
      _context(nullptr),
      _textStream(nullptr),
      _textLayout(nullptr),
      _renderFrameLimiter(nullptr) {
  _ASSERTE(host);

  host->GetGame(_game.Assign());
  host->GetTimer(_timer.Assign());

  std::string pref;
  if (!GetPreference(_game, PreferenceConstants::SIM_FPS, pref)) {
    FATALERROR(_host,
               PreferenceConstants::SIM_FPS << " was not found in preferences");
  }

  _awaitFrame.test_and_set();

  UINT32 simulationFps = FromString<UINT32>(pref);
  if (!simulationFps) {
    FATALERROR(_host, PreferenceConstants::SIM_FPS << " is not an integer");
  }

  if (MGDF_OK != FrameLimiter::TryCreate(simulationFps, &_simFrameLimiter)) {
    FATALERROR(_host, "Unable to create sim frame limiter");
  }

  if (GetPreference(_game, PreferenceConstants::RENDER_FPS, pref)) {
    if (MGDF_OK != FrameLimiter::TryCreate(FromString<UINT32>(pref),
                                           &_renderFrameLimiter)) {
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

  if (FAILED(DWriteCreateFactory(
          DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory1),
          reinterpret_cast<IUnknown **>(&_dWriteFactory)))) {
    FATALERROR(_host, "Unable to create IDWriteFactory");
  }

  _textStream = new TextStream(_dWriteFactory);

  IDWriteFontCollection *fontCollection;
  if (FAILED(_dWriteFactory->GetSystemFontCollection(&fontCollection))) {
    FATALERROR(_host, "Unable to get  font collection");
  }

  if (FAILED(_dWriteFactory->CreateTextFormat(
          L"Arial", fontCollection, DWRITE_FONT_WEIGHT_NORMAL,
          DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 14, L"",
          &_textFormat))) {
    FATALERROR(_host, "Unable to create text format");
  }

  SAFE_RELEASE(fontCollection);
}

MGDFApp::~MGDFApp() {
  delete _simFrameLimiter;
  delete _renderFrameLimiter;
  delete _textStream;
  SAFE_RELEASE(_context);
  SAFE_RELEASE(_blackBrush);
  SAFE_RELEASE(_whiteBrush);
  SAFE_RELEASE(_textFormat);
  SAFE_RELEASE(_textLayout);
  SAFE_RELEASE(_dWriteFactory);
}

UINT32 MGDFApp::GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                              UINT32 *featureLevelsSize) {
  return _host->GetCompatibleD3DFeatureLevels(levels, featureLevelsSize);
}

void MGDFApp::OnInitDevices(HWND window, ID3D11Device *d3dDevice,
                            ID2D1Device *d2dDevice, IDXGIAdapter1 *adapter) {
  _ASSERTE(d3dDevice);
  _ASSERTE(d2dDevice);
  _ASSERTE(adapter);

  if (FAILED(d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
                                            &_context))) {
    FATALERROR(this, "Unable to create ID2D1DeviceContext");
  }

  _host->RTSetDevices(window, d3dDevice, d2dDevice, adapter);
}

bool MGDFApp::IsBackBufferChangePending() {
  return _host->GetRenderSettingsImpl().IsBackBufferChangePending();
}

bool MGDFApp::VSyncEnabled() const {
  return _host->GetRenderSettingsImpl().GetVSync();
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

FullScreenDesc MGDFApp::OnResetSwapChain(
    DXGI_SWAP_CHAIN_DESC1 &swapDesc,
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreenDesc, const RECT &windowSize) {
  _host->GetRenderSettingsImpl().OnResetSwapChain(swapDesc, fullscreenDesc,
                                                  windowSize);
  FullScreenDesc desc;
  _host->GetRenderSettingsImpl().GetFullscreen(&desc);
  return desc;
}

void MGDFApp::OnResize(UINT32 width, UINT32 height) {
  _host->GetRenderSettingsImpl().OnResize(width, height);
}

void MGDFApp::OnBeforeDeviceReset() {
  SAFE_RELEASE(_context);
  SAFE_RELEASE(_blackBrush);
  SAFE_RELEASE(_whiteBrush);
  SAFE_RELEASE(_textLayout)
  _textStream->ClearBrushes();
  _host->RTBeforeDeviceReset();
}

void MGDFApp::OnBeforeBackBufferChange() {
  _context->SetTarget(nullptr);
  _host->RTBeforeBackBufferChange();
}

void MGDFApp::OnBackBufferChange(ID3D11Texture2D *backBuffer,
                                 ID3D11Texture2D *depthStencilBuffer) {
  _ASSERTE(backBuffer);
  _ASSERTE(depthStencilBuffer);

  _host->RTBackBufferChange(backBuffer, depthStencilBuffer);
  _host->SetBackBufferRenderTarget(_context);
}

void MGDFApp::OnBeforeFirstDraw() {
  _renderStart = _activeRenderEnd = _timer->GetCurrentTimeTicks();

  // wait for one sim frame to finish before
  // we start drawing
  while (_awaitFrame.test_and_set()) {
    Sleep(1);
  }
  _host->RTBeforeFirstDraw();
}

void MGDFApp::OnDraw() {
  LARGE_INTEGER currentTime = _renderFrameLimiter
                                  ? _renderFrameLimiter->LimitFps()
                                  : _timer->GetCurrentTimeTicks();

  double elapsedTime =
      _timer->ConvertDifferenceToSeconds(currentTime, _renderStart);
  _stats.AppendRenderTimes(elapsedTime, _timer->ConvertDifferenceToSeconds(
                                            _activeRenderEnd, _renderStart));
  _renderStart = currentTime;

  _host->RTDraw(elapsedTime);

  if (_host->GetDebugImpl()->IsShown()) {
    DrawSystemOverlay();
  }
  _activeRenderEnd = _timer->GetCurrentTimeTicks();
}

void MGDFApp::DrawSystemOverlay() {
  if (!_whiteBrush) {
    InitBrushes();
  }

  _textStream->ClearText();
  _host->GetDebugImpl()->DumpInfo(_stats, *_textStream);

  auto prevLayout = _textLayout;

  if (FAILED(_textStream->GenerateLayout(
          _context, _textFormat,
          static_cast<float>(_host->GetRenderSettingsImpl().GetScreenX()),
          static_cast<float>(_host->GetRenderSettingsImpl().GetScreenY()),
          &_textLayout))) {
    FATALERROR(_host, "Unable to create text layout");
  }

  SecureZeroMemory(&_textMetrics, sizeof(_textMetrics));
  if (FAILED(_textLayout->GetMetrics(&_textMetrics))) {
    FATALERROR(_host, "Unable to get text overhang metrics");
  }

  _context->BeginDraw();

  const float margin = 5.0f;
  D2D1_ROUNDED_RECT rect;
  rect.radiusX = margin;
  rect.radiusY = margin;
  rect.rect.top = margin;
  rect.rect.left = margin;
  rect.rect.bottom = (margin * 3) + _textMetrics.height;
  rect.rect.right = (margin * 3) + _textMetrics.width;
  _context->FillRoundedRectangle(&rect, _blackBrush);
  _context->DrawRoundedRectangle(&rect, _whiteBrush);

  D2D_POINT_2F origin;
  origin.x = (2 * margin);
  origin.y = (2 * margin);
  _context->DrawTextLayout(origin, _textLayout, _whiteBrush);

  _context->EndDraw();
}

void MGDFApp::InitBrushes() {
  D2D1_COLOR_F color;
  color.a = color.r = color.g = color.b = 1.0f;
  if (FAILED(_context->CreateSolidColorBrush(color, &_whiteBrush))) {
    FATALERROR(_host, "Unable to create white color brush");
  }

  color.r = color.g = color.b = 0.05f;
  color.a = 0.85f;
  if (FAILED(_context->CreateSolidColorBrush(color, &_blackBrush))) {
    FATALERROR(_host, "Unable to create black color brush");
  }
}

void MGDFApp::OnUpdateSim() {
  if (!_initialized) {
    _simulationEnd = _timer->GetCurrentTimeTicks();
    LOG("Creating Module...", LOG_LOW);
    _host->STCreateModule();
    _initialized = true;
  }
  LARGE_INTEGER simulationEnd = _simulationEnd;

  // execute one frame of game logic as per the current module
  _host->STUpdate(_stats.ExpectedSimTime(), _stats);
  _awaitFrame.clear();

  const LARGE_INTEGER activeSimulationEnd = _timer->GetCurrentTimeTicks();
  _stats.AppendActiveSimTime(
      _timer->ConvertDifferenceToSeconds(activeSimulationEnd, simulationEnd));

  // wait until the next frame to begin if we have any spare time left over
  _simulationEnd = _simFrameLimiter->LimitFps();
  _stats.AppendSimTime(
      _timer->ConvertDifferenceToSeconds(_simulationEnd, simulationEnd));
}

void MGDFApp::OnRawInput(RAWINPUT *input) {
  _ASSERTE(input);
  _host->GetInputManagerImpl()->HandleInput(input);
}

void MGDFApp::OnClearInput() { _host->GetInputManagerImpl()->ClearInput(); }

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

void MGDFApp::OnBeforeHandleMessage() {
  _host->GetInputManagerImpl()->ProcessInput();
}

LRESULT MGDFApp::OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                                 LPARAM lParam) {
  switch (msg) {
    case WM_SYSKEYDOWN:
      switch (wParam) {
        case VK_F12: {
          _host->GetDebugImpl()->ToggleShown();
        } break;
      }
      return 0;
    default:
      return DefWindowProc(hwnd, msg, wParam, lParam);
  }
}

void MGDFApp::FatalError(const char *sender, const char *message) {
  _ASSERTE(sender);
  _ASSERTE(message);
  _host->FatalError(sender, message);
}

}  // namespace core
}  // namespace MGDF