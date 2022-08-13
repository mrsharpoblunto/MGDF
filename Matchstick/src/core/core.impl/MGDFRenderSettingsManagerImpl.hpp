#pragma once

#include <MGDF/MGDF.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <map>
#include <mutex>
#include <vector>

#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

// this class is accessed by the sim and render threads, so setting values and
// doing device resets must be synced up with a mutex
class RenderSettingsManager : public ComBase<IMGDFRenderSettingsManager> {
 public:
  RenderSettingsManager();
  virtual ~RenderSettingsManager();

  UINT32 __stdcall GetMaxFrameLatency() final;
  void __stdcall SetMaxFrameLatency(UINT32 frames) final;

  BOOL __stdcall GetVSync() final;
  void __stdcall SetVSync(BOOL vsync) final;

  void __stdcall GetFullscreen(MGDFFullScreenDesc *desc) final;
  void __stdcall SetFullscreen(const MGDFFullScreenDesc *desc) final;

  UINT64 __stdcall GetMultiSampleLevelCount() final;
  BOOL __stdcall GetMultiSampleLevel(UINT64 index, UINT32 *level) final;

  BOOL __stdcall SetBackBufferMultiSampleLevel(UINT32 multisampleLevel) final;
  UINT32 __stdcall GetBackBufferMultiSampleLevel() final;
  BOOL __stdcall SetCurrentMultiSampleLevel(UINT32 multisampleLevel) final;
  UINT32 __stdcall GetCurrentMultiSampleLevel(UINT32 *quality) final;

  UINT64 __stdcall GetAdaptorModeCount() final;
  BOOL __stdcall GetAdaptorMode(UINT64 index, MGDFAdaptorMode *mode) final;
  BOOL __stdcall GetAdaptorModeFromDimensions(UINT32 width, UINT32 height,
                                              MGDFAdaptorMode *mode) final;
  MGDFAdaptorMode *__stdcall GetCurrentAdaptorMode(MGDFAdaptorMode *mode) final;
  BOOL __stdcall SetCurrentAdaptorMode(const MGDFAdaptorMode *mode) final;
  BOOL __stdcall SetCurrentAdaptorModeToNative(MGDFAdaptorMode *mode) final;
  void __stdcall SetWindowSize(UINT32 width, UINT32 height) final;

  UINT32 __stdcall GetScreenX() final;
  UINT32 __stdcall GetScreenY() final;
  void __stdcall ApplySettings() final;

  void __stdcall GetPreferences(IMGDFPreferenceSet **preferences) final;

  void LoadPreferences(const ComObject<IMGDFGame> &game);

  void InitFromDevice(HWND window, const ComObject<ID3D11Device> &d3dDevice,
                      const ComObject<IDXGIAdapter1> &adapter);
  bool IsBackBufferChangePending();
  void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &desc,
                        DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreenDesc,
                        const RECT &windowSize);
  void OnResize(UINT32 width, UINT32 height);

 private:
  void Cleanup();

  std::atomic_bool _changePending;

  std::vector<MGDFAdaptorMode> _adaptorModes;
  MGDFAdaptorMode _currentAdaptorMode;

  std::vector<UINT32> _multiSampleLevels;
  std::map<UINT32, UINT32> _multiSampleQuality;
  UINT32 _currentMultiSampleLevel;
  UINT32 _backBufferMultiSampleLevel;

  UINT32 _screenX, _screenY;
  UINT32 _maxFrameLatency;
  HWND _window;

  bool _vsync;
  MGDFFullScreenDesc _fullScreen;

  mutable std::mutex _mutex;
};

}  // namespace core
}  // namespace MGDF