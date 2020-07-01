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
  UINT64 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                       UINT64 *featureLevelsSize) final;
  bool OnInitWindow(RECT &windowSize) final;
  void OnInitDevices(HWND window, const ComObject<ID3D11Device> &d3dDevice,
                     const ComObject<ID2D1Device> &d2dDevice,
                     const ComObject<IDXGIAdapter1> &adapter) final;
  MGDFFullScreenDesc OnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &,
                                  DXGI_SWAP_CHAIN_FULLSCREEN_DESC &,
                                  const RECT &windowSize) final;
  void OnResize(UINT32 width, UINT32 height) final;
  bool IsBackBufferChangePending() final;
  bool VSyncEnabled() const final;
  void OnBeforeBackBufferChange() final;
  void OnBackBufferChange(
      const ComObject<ID3D11Texture2D> &backBuffer,
      const ComObject<ID3D11Texture2D> &depthStencilBuffer) final;
  void OnBeforeDeviceReset() final;
  void OnDeviceReset() final;
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
  std::unique_ptr<FrameLimiter> _simFrameLimiter;
  std::unique_ptr<FrameLimiter> _renderFrameLimiter;
  ComObject<IMGDFGame> _game;
  ComObject<IMGDFTimer> _timer;
  ComObject<RenderSettingsManager> _settings;

  HostStats _stats;
  LARGE_INTEGER _renderStart;
  LARGE_INTEGER _activeRenderEnd;
  LARGE_INTEGER _simulationEnd;

  std::atomic_flag _awaitFrame;

  ComObject<ID2D1DeviceContext> _context;
  ComObject<ID2D1SolidColorBrush> _whiteBrush;
  ComObject<ID2D1SolidColorBrush> _blackBrush;
  ComObject<IDWriteFactory1> _dWriteFactory;
  ComObject<IDWriteTextFormat> _textFormat;
  std::unique_ptr<TextStream> _textStream;
  ComObject<IDWriteTextLayout> _textLayout;
  DWRITE_TEXT_METRICS _textMetrics;
};

}  // namespace core
}  // namespace MGDF