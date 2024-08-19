#pragma once

#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_6.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <thread>

#include "core.impl/MGDFTimer.hpp"

namespace MGDF {
namespace core {

class D3DAppFramework {
 public:
  D3DAppFramework(HINSTANCE hInstance);
  virtual ~D3DAppFramework();

  void InitWindow(const std::string &caption, WNDPROC windowProcedure);
  LRESULT MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
  INT32 Run();

 protected:
  virtual void OnBeforeFirstDraw() = 0;
  virtual void OnBeforeDeviceReset() = 0;
  virtual void OnDeviceReset() = 0;
  virtual bool OnInitWindow(RECT &windowSize) = 0;
  virtual void OnInitDevices(HWND window,
                             const ComObject<ID3D11Device> &d3dDevice,
                             const ComObject<ID2D1Device> &d2dDevice,
                             const ComObject<IDXGIAdapter1> &adapter) = 0;
  virtual void OnBeforeBackBufferChange() = 0;
  virtual void OnBackBufferChange(
      const ComObject<ID3D11Texture2D> &backBuffer,
      const ComObject<ID3D11Texture2D> &depthStencilBuffer) = 0;
  virtual MGDFFullScreenDesc OnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &,
                                              DXGI_SWAP_CHAIN_FULLSCREEN_DESC &,
                                              const RECT &windowSize) = 0;
  virtual void OnSwapChainCreated(ComObject<IDXGISwapChain1> &swapchain) = 0;
  virtual void OnResize(UINT32 width, UINT32 height) = 0;

  virtual void OnDraw() = 0;

  virtual void OnUpdateSim() = 0;
  virtual void OnDisplayChange(
      const DXGI_OUTPUT_DESC1 &currentOutputDesc, UINT currentDPI,
      ULONG currentSDRWhiteLevel,
      const std::vector<DXGI_MODE_DESC1> &primaryOutputModes) = 0;
  virtual void OnExternalClose() = 0;
  virtual void OnMouseInput(INT32 x, INT32 y) = 0;
  virtual void OnRawInput(RAWINPUT *input) = 0;
  virtual void OnBeforeHandleMessage() = 0;
  virtual LRESULT OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                                  LPARAM lParam) = 0;
  virtual void OnMoveWindow(INT32 x, INT32 y) = 0;

  virtual UINT64 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                               UINT64 *featureLevelsSize) = 0;
  virtual bool IsBackBufferChangePending() = 0;
  virtual bool VSyncEnabled() const = 0;
  virtual void FatalError(const char *sender, const char *message) = 0;
  void CloseWindow();
  void QueueResetDevice();

 private:
  void InitD3D();
  void PrepareToReinitD3D();
  void ReinitD3D();
  void UninitD3D();
  void InitRawInput();
  void CreateSwapChain();
  void ClearBackBuffer();
  void ResizeBackBuffer();
  bool AllowTearing();
  void CheckForDisplayChanges();

  // Application, Windows, and Direct3D data members.
  ComObject<ID3D11Device> _d3dDevice;
  ComObject<ID3D11DeviceContext> _immediateContext;

  ComObject<ID2D1Device> _d2dDevice;
  ComObject<ID2D1Factory1> _d2dFactory;

  ComObject<IDXGISwapChain1> _swapChain;
  UINT _factoryFlags;
  ComObject<IDXGIFactory6> _factory;
  UINT _adapterIndex;

  ComObject<ID3D11RenderTargetView> _renderTargetView;
  ComObject<ID3D11DepthStencilView> _depthStencilView;
  ComObject<ID3D11Texture2D> _depthStencilBuffer;
  ComObject<ID3D11Texture2D> _backBuffer;

  DXGI_SWAP_CHAIN_DESC1 _swapDesc;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC _fullscreenSwapDesc;
  std::vector<D3D_FEATURE_LEVEL> _levels;

  HINSTANCE _applicationInstance;
  HWND _window;
  RECT _windowRect;
  POINT _currentSize;
  DWORD _windowStyle;
  POINT _clientOffset;
  HANDLE _frameWaitableObject;

  std::atomic_bool _resize, _minimized, _awaitingD3DReset;
  std::atomic_flag _runRenderThread;

  std::unique_ptr<std::thread> _renderThread;

  bool _maximized;
  bool _resizing;
  bool _awaitingResize;
  bool _internalShutDown;
  bool _allowTearing;
  MGDFFullScreenDesc _currentFullScreen;
};

// defines a function which calls into an instance of a d3dApp subclass to
// access the wndproc
#define D3DAPP_WNDPROC(wndProcName, className)                         \
  LRESULT CALLBACK wndProcName##(HWND hwnd, UINT32 msg, WPARAM wParam, \
                                 LPARAM lParam) {                      \
    if (##className## != nullptr) {                                    \
      return className##->MsgProc(hwnd, msg, wParam, lParam);          \
    } else {                                                           \
      return DefWindowProc(hwnd, msg, wParam, lParam);                 \
    }                                                                  \
  }

}  // namespace core
}  // namespace MGDF
