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

#include "MGDFGraphicsRequirements.hpp"

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
  BOOL __stdcall SetCurrentPrimaryDisplayMode(
      const MGDFDisplayMode *mode) final;
  void __stdcall SetWindowSize(UINT32 width, UINT32 height) final;
  void __stdcall SetHDREnabled(BOOL enabled) final;
  void __stdcall Cancel() final;
  void __stdcall SetWindowAspectLock(BOOL enabled) final;
  BOOL __stdcall SetLowLatencyMode(MGDFLowLatencyMode mode) final;
  BOOL __stdcall SetFrameGeneration(UINT32 generatedFrames) final;
  BOOL __stdcall SetRayTracing(BOOL enabled) final;

 private:
  bool _cancelled;
  std::optional<MGDFLowLatencyMode> _newLowLatencyMode;
  std::optional<UINT32> _newFrameGeneration;
  std::optional<bool> _newRayTracing;
  std::optional<BOOL> _newWindowAspectLock;
  std::optional<UINT32> _newMaxFrameLatency;
  std::optional<BOOL> _newVsync;
  std::optional<UINT32> _newBackBufferMultiSampleLevel;
  std::optional<UINT32> _newCurrentMultiSampleLevel;
  std::optional<MGDFFullScreenDesc> _newFullscreen;
  std::optional<BOOL> _newHDREnabled;
  std::optional<MGDFDisplayMode> _newPrimaryDisplayMode;
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
  UINT64 __stdcall GetPrimaryDisplayModeCount() final;
  BOOL __stdcall GetPrimaryDisplayMode(UINT64 index,
                                       MGDFDisplayMode *mode) final;
  void __stdcall GetPrimaryDisplayModes(MGDFDisplayMode **modes) final;
  BOOL __stdcall GetPrimaryDisplayModeFromDimensions(
      UINT32 width, UINT32 height, MGDFDisplayMode *mode) final;
  BOOL __stdcall GetNativePrimaryDisplayMode(MGDFDisplayMode *mode) final;
  MGDFDisplayMode *__stdcall GetCurrentPrimaryDisplayMode(
      MGDFDisplayMode *mode) final;
  void __stdcall GetCurrentOutputDisplayInfo(MGDFOutputDisplayInfo *info) final;
  UINT __stdcall GetCurrentOutputDPI() final;
  BOOL __stdcall GetHDREnabled() final;
  void __stdcall CreatePendingSettingsChange(
      IMGDFPendingRenderSettingsChange **) final;
  UINT32 __stdcall GetScreenX() final;
  UINT32 __stdcall GetScreenY() final;
  void __stdcall GetPreferences(IMGDFPreferenceSet **preferences) final;

  BOOL __stdcall GetWindowAspectLock() final;
  BOOL __stdcall GetWindowResizeEnabled() final;
  BOOL __stdcall GetWindowSizeLimits(UINT32 *width, UINT32 *height) final;
  UINT64 __stdcall GetUpscaleModeCount() final;
  BOOL __stdcall GetUpscaleMode(UINT64 index, MGDFUpscaleMode *mode) final;
  UINT32 __stdcall GetMaxGeneratedFrames() final;
  UINT64 __stdcall GetLowLatencyModeCount() final;
  BOOL __stdcall GetLowLatencyMode(UINT64 index,
                                   MGDFLowLatencyMode *mode) final;
  MGDFLowLatencyMode __stdcall GetCurrentLowLatencyMode() final;
  UINT32 __stdcall GetFrameGeneration() final;
  BOOL __stdcall GetRayTracingSupport(MGDFRayTracingSupport *support) final;
  BOOL __stdcall GetRayTracing() final;
  std::optional<std::pair<UINT32, UINT32>> TakeWindowSize();
  // Call after each device creation. Clamps the settings to the support.
  void SetGraphicsSupport(GraphicsSupport support);
  // Call before InitFromDevice.
  void SetHDRFormat(MGDFHDRFormat format);

  void LoadPreferences(const ComObject<IMGDFGame> &game);
  void InitFromDevice(const ComObject<ID3D11Device> &d3dDevice);
  void InitD3D12();
  bool IsBackBufferChangePending();
  void OnResetSwapChain(DXGI_SWAP_CHAIN_DESC1 &desc,
                        DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreenDesc,
                        DXGI_COLOR_SPACE_TYPE &colorSpace,
                        const RECT &windowSize);
  void OnResize(UINT32 width, UINT32 height);
  void SetOutputProperties(const MGDFOutputDisplayInfo &info, UINT currentDPI,
                           const std::vector<MGDFDisplayMode> &primaryModes);

 private:
  void Cleanup();
  void ClampToSupportLocked();
  MGDFDisplayMode GetBestAlternativeDisplayMode();

  std::atomic_bool _changePending{false};
  bool _windowAspectLock = false;
  bool _windowResizeEnabled = false;
  std::optional<std::pair<UINT32, UINT32>> _pendingWindowSize;

  std::vector<MGDFDisplayMode> _primaryDisplayModes;
  MGDFDisplayMode _primaryDisplayMode;

  std::vector<UINT32> _multiSampleLevels;
  std::map<UINT32, UINT32> _multiSampleQuality;
  UINT32 _currentMultiSampleLevel;
  UINT32 _backBufferMultiSampleLevel;
  DXGI_FORMAT _sdrBackBufferFormat;
  DXGI_FORMAT _hdrBackBufferFormat;
  DXGI_COLOR_SPACE_TYPE _hdrColorSpace;

  UINT32 _screenX, _screenY;
  UINT32 _maxFrameLatency;

  GraphicsSupport _support;
  MGDFLowLatencyMode _lowLatencyMode = MGDF_LOW_LATENCY_OFF;
  UINT32 _frameGeneration = 0;
  bool _rayTracing = false;

  UINT _currentOutputDPI;
  MGDFOutputDisplayInfo _currentOutputDisplayInfo;
  bool _hdrEnabled;
  bool _vsync;
  MGDFFullScreenDesc _fullScreen;

  mutable std::mutex _mutex;
};

}  // namespace core
}  // namespace MGDF
