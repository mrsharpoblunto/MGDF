#pragma once

#include <MGDF/MGDF.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <map>
#include <mutex>
#include <optional>
#include <vector>

namespace MGDF {
namespace core {

class RenderSettingsManager;

class PendingRenderSettingsChange
    : public ComBase<IMGDFPendingRenderSettingsChange> {
 public:
  PendingRenderSettingsChange(RenderSettingsManager &manager)
      : _cancelled(false), _manager(manager) {}
  virtual ~PendingRenderSettingsChange();

  void __stdcall SetMaxFrameLatency(UINT32 frames) final;
  void __stdcall SetVSync(BOOL vsync) final;
  void __stdcall SetFullscreen(const MGDFFullScreenDesc *fullscreen) final;
  BOOL __stdcall SetBackBufferMultiSampleLevel(UINT32 multisampleLevel) final;
  BOOL __stdcall SetCurrentMultiSampleLevel(UINT32 multisampleLevel) final;
  BOOL __stdcall SetCurrentDisplayMode(const MGDFDisplayMode *mode) final;
  void __stdcall SetWindowSize(UINT32 width, UINT32 height) final;
  void __stdcall SetHDREnabled(BOOL enabled) final;
  void __stdcall Cancel() final;

 private:
  bool _cancelled;
  std::optional<UINT32> _newMaxFrameLatency;
  std::optional<BOOL> _newVsync;
  std::optional<UINT32> _newBackBufferMultiSampleLevel;
  std::optional<UINT32> _newCurrentMultiSampleLevel;
  std::optional<MGDFFullScreenDesc> _newFullscreen;
  std::optional<BOOL> _newHDREnabled;
  std::optional<MGDFDisplayMode> _newDisplayMode;
  std::optional<std::pair<UINT32, UINT32>> _newSize;
  RenderSettingsManager &_manager;
};

// this class is accessed by the sim and render threads, so setting values and
// doing device resets must be synced up with a mutex
class RenderSettingsManager : public ComBase<IMGDFRenderSettingsManager> {
 public:
  friend class PendingRenderSettingsChange;
  RenderSettingsManager();
  virtual ~RenderSettingsManager();

  DXGI_FORMAT __stdcall GetSDRBackBufferFormat() final;
  DXGI_FORMAT __stdcall GetHDRBackBufferFormat() final;
  UINT32 __stdcall GetMaxFrameLatency() final;
  BOOL __stdcall GetVSync() final;
  void __stdcall GetFullscreen(MGDFFullScreenDesc *desc) final;
  UINT64 __stdcall GetMultiSampleLevelCount() final;
  BOOL __stdcall GetMultiSampleLevel(UINT64 index, UINT32 *level) final;
  UINT32 __stdcall GetBackBufferMultiSampleLevel() final;
  UINT32 __stdcall GetCurrentMultiSampleLevel(UINT32 *quality) final;
  UINT64 __stdcall GetDisplayModeCount() final;
  BOOL __stdcall GetDisplayMode(UINT64 index, MGDFDisplayMode *mode) final;
  void __stdcall GetDisplayModes(MGDFDisplayMode **modes) final;
  BOOL __stdcall GetDisplayModeFromDimensions(UINT32 width, UINT32 height,
                                              MGDFDisplayMode *mode) final;
  BOOL __stdcall GetNativeDisplayMode(MGDFDisplayMode *mode) final;
  MGDFDisplayMode *__stdcall GetCurrentDisplayMode(MGDFDisplayMode *mode) final;
  BOOL __stdcall GetCurrentOutputHDRDisplayInfo(MGDFHDRDisplayInfo *info) final;
  UINT __stdcall GetCurrentOutputDPI() final;
  BOOL __stdcall GetHDREnabled() final;
  void __stdcall CreatePendingSettingsChange(
      IMGDFPendingRenderSettingsChange **) final;
  UINT32 __stdcall GetScreenX() final;
  UINT32 __stdcall GetScreenY() final;
  void __stdcall GetPreferences(IMGDFPreferenceSet **preferences) final;

  void LoadPreferences(const ComObject<IMGDFGame> &game);
  void InitFromDevice(const ComObject<ID3D11Device> &d3dDevice);
  bool IsBackBufferChangePending();
  void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &desc,
                        DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreenDesc,
                        const RECT &windowSize);
  void OnResize(UINT32 width, UINT32 height);
  void SetOutputProperties(const MGDFHDRDisplayInfo &info, UINT currentDPI,
                           const std::vector<MGDFDisplayMode> &modes);

 private:
  void Cleanup();
  MGDFDisplayMode GetBestAlternativeDisplayMode();

  std::atomic_bool _changePending;

  std::vector<MGDFDisplayMode> _displayModes;
  MGDFDisplayMode _currentDisplayMode;

  std::vector<UINT32> _multiSampleLevels;
  std::map<UINT32, UINT32> _multiSampleQuality;
  UINT32 _currentMultiSampleLevel;
  UINT32 _backBufferMultiSampleLevel;
  DXGI_FORMAT _sdrBackBufferFormat;
  DXGI_FORMAT _hdrBackBufferFormat;

  UINT32 _screenX, _screenY;
  UINT32 _maxFrameLatency;

  UINT _currentDPI;
  MGDFHDRDisplayInfo _currentDisplayInfo;
  bool _hdrEnabled;
  bool _vsync;
  MGDFFullScreenDesc _fullScreen;

  mutable std::mutex _mutex;
};

}  // namespace core
}  // namespace MGDF