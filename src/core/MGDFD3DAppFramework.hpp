#pragma once

#include <d3d11.h>
#include <dxgi1_6.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "MGDFRenderBackend.hpp"

namespace MGDF {
namespace core {

class D3D11RenderBackend;

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

  void InitWindow(const std::string &caption);
  LRESULT MsgProc(HWND hwnd, UINT32 msg, WPARAM wParam, LPARAM lParam);
  INT32 Run();

 protected:
  virtual std::pair<DXGI_FORMAT, DXGI_FORMAT>
  RTOnBeforeEnumerateDisplayModes() = 0;
  virtual void RTOnBeforeFirstDraw() = 0;
  virtual void RTOnBeforeDeviceReset() = 0;
  virtual void RTOnDeviceReset() = 0;
  virtual void RTOnInitDevice(const ComObject<ID3D11Device> &d3dDevice) = 0;
  virtual void RTOnBeforeBackBufferChange() = 0;
  virtual void RTOnBackBufferChange(
      const ComObject<ID3D11Texture2D> &backBuffer,
      const ComObject<ID3D11Texture2D> &depthStencilBuffer) = 0;
  virtual MGDFFullScreenDesc RTOnResetSwapChain(
      DXGI_SWAP_CHAIN_DESC1 &, DXGI_SWAP_CHAIN_FULLSCREEN_DESC &,
      const RECT &windowSize) = 0;
  virtual std::optional<UINT> RTGetMaxFrameLatency() const = 0;
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
  virtual void OnRawInput(std::function<RAWINPUT *()> getInput) = 0;
  virtual LRESULT OnHandleMessage(HWND hwnd, UINT32 msg, WPARAM wParam,
                                  LPARAM lParam) = 0;
  virtual void OnMoveWindow(INT32 x, INT32 y) = 0;
  virtual bool OnSetCursor() = 0;
  virtual UINT64 GetCompatibleD3DFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                               UINT64 *featureLevelsSize) = 0;
  virtual void FatalError(const char *sender, const char *message) = 0;

  void CloseWindow();
  void QueueResetDevice();
  void RTWaitForGpuIdle();

 private:
  ComObject<IDXGIFactory6> RTCreateDXGIFactory();
  bool RTInitD3D(const HWND window);
  void RTPrepareToReinitD3D();
  void RTReinitD3D(const HWND window);
  void RTUninitD3D();
  void RTCreateSwapChain(const HWND window);
  void ApplyWindowMode(const HWND window, const bool fullScreenBorderless);
  void RTResizeBackBuffer();
  bool RTAllowTearing();
  bool RTCheckForDisplayChanges(const HWND window);

  void InitRawInput();
  void ProcessRawInput();
  void PushRTMessage(
      DisplayChangeType type,
      std::function<void(DisplayChangeMessage &)> genMessage = nullptr);
  bool PopRTMessage(std::optional<DisplayChangeMessage> &message);

  // Render thread variables
  ComObject<IDXGIFactory6> _rtFactory;
  ComObject<IDXGIAdapter> _rtAdapter;
  std::unique_ptr<IRenderBackend> _rtRenderBackend;
  D3D11RenderBackend *_rtD3D11Backend = nullptr;
  DXGI_SWAP_CHAIN_DESC1 _rtSwapDesc;
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC _rtFullscreenSwapDesc;
  MGDFFullScreenDesc _rtCurrentFullScreen;
  bool _rtAllowTearing;
  RECT _rtWindowRect;

  // shared variables
  std::mutex _displayChangeMutex;
  std::list<DisplayChangeMessage> _pendingDisplayChanges;
  std::atomic_bool _minimized, _awaitingD3DReset, _hasSwapChain;
  std::atomic_flag _runRenderThread;

  // Non-render thread variables
  std::unique_ptr<std::thread> _renderThread;
  HINSTANCE _applicationInstance;
  HWND _window;
  DWORD _windowStyle;
  POINT _clientOffset;
  std::unique_ptr<POINT> _resizing;
  bool _internalShutDown;
  std::vector<BYTE> _rawInputBuffer;
};

}  // namespace core
}  // namespace MGDF
