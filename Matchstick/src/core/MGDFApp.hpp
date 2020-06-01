#pragma once

#include <dwrite_1.h>

#include "MGDFD3DAppFramework.hpp"
#include "MGDFFrameLimiter.hpp"
#include "core.impl/MGDFHostImpl.hpp"
#include "core.impl/MGDFHostStats.hpp"
#include "core.impl/MGDFTextStream.hpp"

namespace MGDF {
namespace core {

class MGDFApp : public D3DAppFramework {
 public:
  MGDFApp(Host *host, HINSTANCE hInstance);
  virtual ~MGDFApp();

 protected:
  UINT32 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                       UINT32 *featureLevelsSize) final;
  bool OnInitWindow(RECT &windowSize) final;
  void OnInitDevices(HWND window, ID3D11Device *d3dDevice,
                     ID2D1Device *d2dDevice, IDXGIAdapter1 *adapter) final;
  FullScreenDesc OnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &,
                                  DXGI_SWAP_CHAIN_FULLSCREEN_DESC &,
                                  const RECT &windowSize) final;
  void OnResize(UINT32 width, UINT32 height) final;
  bool IsBackBufferChangePending() final;
  bool VSyncEnabled() const final;
  void OnBeforeBackBufferChange() final;
  void OnBackBufferChange(ID3D11Texture2D *backBuffer,
                          ID3D11Texture2D *depthStencilBuffer) final;
  void OnBeforeDeviceReset() final;
  void OnBeforeFirstDraw() final;
  void OnDraw() final;

  void OnUpdateSim() final;

  void OnBeforeHandleMessage() final;
  LRESULT OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                          LPARAM lParam) final;
  void OnExternalClose() final;
  void OnRawInput(RAWINPUT *input) final;
  void OnClearInput() final;
  void OnMouseInput(INT32 x, INT32 y) final;
  void OnMoveWindow(INT32 x, INT32 y) final;

  void FatalError(const char *sender, const char *message) final;

 private:
  void DrawSystemOverlay();
  void InitBrushes();

  bool _initialized;

  Host *_host;
  FrameLimiter *_simFrameLimiter;
  FrameLimiter *_renderFrameLimiter;
  ComObject<IGame> _game;
  ComObject<ITimer> _timer;
  ComObject<RenderSettingsManager> _settings;

  HostStats _stats;
  LARGE_INTEGER _renderStart;
  LARGE_INTEGER _activeRenderEnd;
  LARGE_INTEGER _simulationEnd;

  std::atomic_flag _awaitFrame;

  ID2D1DeviceContext *_context;
  ID2D1SolidColorBrush *_whiteBrush;
  ID2D1SolidColorBrush *_blackBrush;
  IDWriteFactory1 *_dWriteFactory;
  IDWriteTextFormat *_textFormat;
  TextStream *_textStream;
  IDWriteTextLayout *_textLayout;
  DWRITE_TEXT_METRICS _textMetrics;
};

}  // namespace core
}  // namespace MGDF