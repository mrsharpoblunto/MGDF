#pragma once

#include <dwrite_1.h>

#include "MGDFD3DAppFramework.hpp"
#include "MGDFFrameLimiter.hpp"
#include "core.impl/MGDFHostImpl.hpp"
#include "core.impl/MGDFHostMetrics.hpp"
#include "core.impl/MGDFTextStream.hpp"

namespace MGDF {
namespace core {

class MGDFApp : public D3DAppFramework {
 public:
  MGDFApp(ComObject<Host> &host, HINSTANCE hInstance);
  virtual ~MGDFApp();

 protected:
  std::pair<DXGI_FORMAT, DXGI_FORMAT> RTOnBeforeEnumerateDisplayModes() final;
  void RTOnInitDevices(const ComObject<ID3D11Device> &d3dDevice,
                       const ComObject<ID2D1Device> &d2dDevice) final;
  MGDFFullScreenDesc RTOnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &,
                                        DXGI_SWAP_CHAIN_FULLSCREEN_DESC &,
                                        const RECT &windowSize) final;
  void RTOnSwapChainCreated(ComObject<IDXGISwapChain1> &swapchain) final;
  void RTOnResize(UINT32 width, UINT32 height) final;
  bool RTIsBackBufferChangePending() final;
  bool RTVSyncEnabled() const final;
  void RTOnBeforeBackBufferChange() final;
  void RTOnBackBufferChange(
      const ComObject<ID3D11Texture2D> &backBuffer,
      const ComObject<ID3D11Texture2D> &depthStencilBuffer) final;
  void RTOnBeforeDeviceReset() final;
  void RTOnDeviceReset() final;
  void RTOnBeforeFirstDraw() final;
  void RTOnDisplayChange(
      const DXGI_OUTPUT_DESC1 &currentOutputDesc, UINT currentDPI,
      ULONG currentSDRWhiteLevel,
      const std::vector<DXGI_MODE_DESC1> &primaryOutputModes) final;
  void RTOnDraw() final;

  void STOnUpdateSim() final;

  UINT64 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                       UINT64 *featureLevelsSize) final;
  bool OnInitWindow(RECT &windowSize) final;
  bool OnHideCursor() final;
  LRESULT OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                          LPARAM lParam) final;
  void OnExternalClose() final;
  void OnRawInput(std::function<RAWINPUT *()> input) final;
  void OnMouseInput(INT32 x, INT32 y) final;
  void OnMoveWindow(INT32 x, INT32 y) final;

  void FatalError(const char *sender, const char *message) final;

 private:
  void RTDrawSystemOverlay();
  void RTInitBrushes();
  void RTInitDirectWrite();

  bool _stInitialized;
  std::unique_ptr<FrameLimiter> _stFrameLimiter;
  LARGE_INTEGER _stEnd;

  std::unique_ptr<FrameLimiter> _rtFrameLimiter;
  LARGE_INTEGER _rtStart;
  LARGE_INTEGER _rtActiveEnd;
  ComObject<ID2D1DeviceContext> _rtContext;
  ComObject<ID2D1SolidColorBrush> _rtWhiteBrush;
  ComObject<ID2D1SolidColorBrush> _rtBlackBrush;
  ULONG _sdrReferenceWhiteLevel;
  ComObject<IDWriteFactory1> _rtDWriteFactory;
  ComObject<IDWriteTextFormat> _rtTextFormat;
  std::unique_ptr<TextStream> _rtTextStream;
  ComObject<IDWriteTextLayout> _rtTextLayout;
  DWRITE_TEXT_METRICS _rtTextMetrics;

  ComObject<Host> _host;
  ComObject<IMGDFGame> _game;
  ComObject<IMGDFTimer> _timer;
  ComObject<RenderSettingsManager> _settings;
  HostMetrics _metrics;
  std::atomic_flag _awaitFrame;
};

}  // namespace core
}  // namespace MGDF