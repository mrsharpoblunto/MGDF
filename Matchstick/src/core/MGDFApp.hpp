#pragma once

#include <dwrite_1.h>

#include "MGDFD3DAppFramework.hpp"
#include "MGDFFrameLimiter.hpp"
#include "core.impl/MGDFHostImpl.hpp"
#include "core.impl/MGDFHostStats.hpp"

namespace MGDF {
namespace core {

class MGDFApp : public D3DAppFramework {
 public:
  MGDFApp(Host *host, HINSTANCE hInstance);
  virtual ~MGDFApp();

 protected:
  UINT32 GetCompatibleD3DFeatureLevels(
      D3D_FEATURE_LEVEL *levels, UINT32 *featureLevelsSize) override final;
  bool OnInitWindow(RECT &windowSize) override final;
  void OnInitDevices(HWND window, ID3D11Device *d3dDevice,
                     ID2D1Device *d2dDevice,
                     IDXGIAdapter1 *adapter) override final;
  FullScreenDesc OnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &,
                                  DXGI_SWAP_CHAIN_FULLSCREEN_DESC &,
                                  const RECT &windowSize) override final;
  void OnResize(UINT32 width, UINT32 height) override final;
  bool IsBackBufferChangePending() override final;
  bool VSyncEnabled() const override final;
  void OnBeforeBackBufferChange() override final;
  void OnBackBufferChange(ID3D11Texture2D *backBuffer,
                          ID3D11Texture2D *depthStencilBuffer) override final;
  void OnBeforeDeviceReset() override final;
  void OnBeforeFirstDraw() override final;
  void OnDraw() override final;

  void OnUpdateSim() override final;

  void OnBeforeHandleMessage() override final;
  LRESULT OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                          LPARAM lParam) override final;
  void OnExternalClose() override final;
  void OnRawInput(RAWINPUT *input) override final;
  void OnClearInput() override final;
  void OnMouseInput(INT32 x, INT32 y) override final;
  void OnMoveWindow(INT32 x, INT32 y) override final;

  void FatalError(const char *sender, const char *message) override final;

 private:
  void DrawSystemOverlay();
  void InitBrushes();

  bool _initialized;

  Host *_host;
  FrameLimiter *_simFrameLimiter;
  FrameLimiter *_renderFrameLimiter;

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
};

}  // namespace core
}  // namespace MGDF