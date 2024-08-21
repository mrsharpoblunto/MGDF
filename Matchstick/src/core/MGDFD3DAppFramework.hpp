#pragma once

#include <d3d11.h>
#include <dxgi1_6.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

namespace MGDF {
namespace core {

enum DisplayChangeType {
  DC_WINDOW_MOVE,
  DC_WINDOW_RESIZE,
  DC_WINDOW_MAXIMIZE,
  DC_DISPLAY_CHANGE,
};

struct DisplayChangeMessage {
  DisplayChangeMessage(DisplayChangeType type) : Type(type), Point({}) {}
  DisplayChangeType Type;
  POINT Point;
};

class D3DAppFramework {
 public:
  D3DAppFramework(HINSTANCE hInstance);
  virtual ~D3DAppFramework();

  void InitWindow(const std::string &caption, WNDPROC windowProcedure);
  LRESULT MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
  INT32 Run();

 protected:
  virtual void RTOnBeforeFirstDraw() = 0;
  virtual void RTOnBeforeDeviceReset() = 0;
  virtual void RTOnDeviceReset() = 0;
  virtual void RTOnInitDevices(HWND window,
                               const ComObject<ID3D11Device> &d3dDevice,
                               const ComObject<ID2D1Device> &d2dDevice,
                               const ComObject<IDXGIAdapter1> &adapter) = 0;
  virtual void RTOnBeforeBackBufferChange() = 0;
  virtual void RTOnBackBufferChange(
      const ComObject<ID3D11Texture2D> &backBuffer,
      const ComObject<ID3D11Texture2D> &depthStencilBuffer) = 0;
  virtual MGDFFullScreenDesc RTOnResetSwapChain(
      DXGI_SWAP_CHAIN_DESC1 &, DXGI_SWAP_CHAIN_FULLSCREEN_DESC &,
      const RECT &windowSize) = 0;
  virtual void RTOnSwapChainCreated(ComObject<IDXGISwapChain1> &swapchain) = 0;
  virtual void RTOnDisplayChange(
      const DXGI_OUTPUT_DESC1 &currentOutputDesc, UINT currentDPI,
      ULONG currentSDRWhiteLevel,
      const std::vector<DXGI_MODE_DESC1> &primaryOutputModes) = 0;
  virtual void RTOnResize(UINT32 width, UINT32 height) = 0;
  virtual bool RTVSyncEnabled() const = 0;
  virtual void RTOnDraw() = 0;
  virtual bool RTIsBackBufferChangePending() = 0;

  virtual void STOnUpdateSim() = 0;

  virtual bool OnInitWindow(RECT &windowSize) = 0;
  virtual void OnExternalClose() = 0;
  virtual void OnMouseInput(INT32 x, INT32 y) = 0;
  virtual void OnRawInput(RAWINPUT *input) = 0;
  virtual LRESULT OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                                  LPARAM lParam) = 0;
  virtual void OnMoveWindow(INT32 x, INT32 y) = 0;
  virtual bool OnHideCursor() = 0;
  virtual UINT64 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                               UINT64 *featureLevelsSize) = 0;
  virtual void FatalError(const char *sender, const char *message) = 0;

  void CloseWindow();
  void QueueResetDevice();

 private:
  ComObject<IDXGIFactory6> RTCreateDXGIFactory();
  void RTInitD3D(const HWND window);
  void RTPrepareToReinitD3D();
  void RTReinitD3D(const HWND window);
  void RTUninitD3D();
  void RTCreateSwapChain(const HWND window);
  void RTClearBackBuffer();
  void RTResizeBackBuffer();
  bool RTAllowTearing();
  void RTCheckForDisplayChanges(const HWND window);

  void InitRawInput();
  void PushRTMessage(
      DisplayChangeType type,
      std::function<void(DisplayChangeMessage &)> genMessage = nullptr);
  bool PopRTMessage(std::unique_ptr<DisplayChangeMessage> &message);

  // Render thread variables
  ComObject<ID3D11Device> _rtD3dDevice;
  ComObject<ID3D11DeviceContext> _rtImmediateContext;
  ComObject<ID2D1Device> _rtD2dDevice;
  ComObject<ID2D1Factory1> _rtD2dFactory;
  ComObject<IDXGISwapChain1> _rtSwapChain;
  ComObject<IDXGIFactory6> _rtFactory;
  ComObject<ID3D11RenderTargetView> _rtRenderTargetView;
  ComObject<ID3D11DepthStencilView> _rtDepthStencilView;
  ComObject<ID3D11Texture2D> _rtDepthStencilBuffer;
  ComObject<ID3D11Texture2D> _rtBackBuffer;
  DXGI_SWAP_CHAIN_DESC1 _rtSwapDesc;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC _rtFullscreenSwapDesc;
  std::vector<D3D_FEATURE_LEVEL> _rtLevels;
  HANDLE _rtFrameWaitableObject;
  MGDFFullScreenDesc _rtCurrentFullScreen;
  bool _rtAllowTearing;
  RECT _rtWindowRect;

  // shared variables
  std::mutex _displayChangeMutex;
  std::list<DisplayChangeMessage> _pendingDisplayChanges;
  std::atomic_bool _minimized, _awaitingD3DReset;
  std::atomic_flag _runRenderThread;

  // Non-render thread variables
  std::unique_ptr<std::thread> _renderThread;
  HINSTANCE _applicationInstance;
  HWND _window;
  DWORD _windowStyle;
  POINT _clientOffset;
  std::unique_ptr<POINT> _resizing;
  bool _internalShutDown;
};

// defines a function which calls into an instance of a d3dApp subclass to
// access the wndproc
#define D3DAPP_WNDPROC(wndProcName, className)                         \
  LRESULT CALLBACK wndProcName##(HWND hwnd, UINT32 msg, WPARAM wParam, \
                                 LPARAM lParam) {                      \
    if (##className## != nullptr) {                                    \
      return className##->MsgProc(hwnd, msg, wParam, lParam);          \
    } else {                                                           \
      return ::DefWindowProc(hwnd, msg, wParam, lParam);               \
    }                                                                  \
  }

}  // namespace core
}  // namespace MGDF
