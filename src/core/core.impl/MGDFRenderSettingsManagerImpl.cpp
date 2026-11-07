#include "StdAfx.h"

#include "MGDFRenderSettingsManagerImpl.hpp"

#include <algorithm>
#include <sstream>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFPreferenceConstants.hpp"
#include "../common/MGDFPreferenceSet.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFOpenXR.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

#define SCREEN_RES(mode)                  \
  mode.Width << "x" << mode.Height << "@" \
             << (mode.RefreshRateNumerator / mode.RefreshRateDenominator) + 1

namespace MGDF {
namespace core {

PendingRenderSettingsChange::~PendingRenderSettingsChange() {
  if (_cancelled) return;

  std::lock_guard<std::mutex> lock(_manager._mutex);
  bool changed = false;

  if (_newMaxFrameLatency.has_value()) {
    changed = true;
    _manager._maxFrameLatency = _newMaxFrameLatency.value();
  }
  if (_newFullscreen.has_value()) {
    changed = true;
    _manager._fullScreen = _newFullscreen.value();
  }
  if (_newVsync.has_value()) {
    changed = true;
    _manager._vsync = _newVsync.value();
  }

  if (_newSize.has_value()) {
    changed = true;
    _manager._pendingWindowSize = _newSize;
  }
  if (_newPrimaryDisplayMode) {
    changed = true;
    _manager._primaryDisplayMode = *_newPrimaryDisplayMode;
  }
  if (_newWindowAspectLock) {
    _manager._windowAspectLock = *_newWindowAspectLock != FALSE;
  }
  if (_newHDREnabled.has_value()) {
    changed = true;
    _manager._hdrEnabled = _newHDREnabled.value();
  }
  if (_newLowLatencyMode) {
    _manager._lowLatencyMode = *_newLowLatencyMode;
  }
  if (_newFrameGeneration &&
      *_newFrameGeneration != _manager._frameGeneration) {
    changed = true;
    _manager._frameGeneration = *_newFrameGeneration;
  }
  if (_newVR) {
    _manager._vrPreference = *_newVR;
    _manager._vrRequest = *_newVR;
    changed = true;
  }
  if (_newRayTracing) {
    _manager._rayTracing = *_newRayTracing;
  }

  if (changed) {
    _manager.MarkChangeLocked();
  }
}

void PendingRenderSettingsChange::SetMaxFrameLatency(UINT32 frames) {
  _newMaxFrameLatency = frames;
}

void PendingRenderSettingsChange::SetVSync(BOOL vsync) { _newVsync = vsync; }

void PendingRenderSettingsChange::SetFullscreen(
    const MGDFFullScreenDesc *fullscreen) {
  _newFullscreen = *fullscreen;
}

BOOL PendingRenderSettingsChange::SetCurrentPrimaryDisplayMode(
    const MGDFDisplayMode *mode) {
  if (!mode) return false;
  std::lock_guard<std::mutex> lock(_manager._mutex);

  for (auto currentMode : _manager._primaryDisplayModes) {
    if (currentMode.Width == mode->Width &&
        currentMode.Height == mode->Height &&
        currentMode.SupportsHDR == mode->SupportsHDR &&
        currentMode.RefreshRateDenominator == mode->RefreshRateDenominator &&
        currentMode.RefreshRateNumerator == mode->RefreshRateNumerator) {
      _newPrimaryDisplayMode = currentMode;
      return true;
    }
  }

  return false;
}

void PendingRenderSettingsChange::SetWindowSize(UINT32 width, UINT32 height) {
  _newSize = std::make_pair(width, height);
}

void PendingRenderSettingsChange::SetHDREnabled(BOOL enabled) {
  _newHDREnabled = enabled;
}

void PendingRenderSettingsChange::SetWindowAspectLock(BOOL enabled) {
  _newWindowAspectLock = enabled;
}

BOOL PendingRenderSettingsChange::SetLowLatencyMode(MGDFLowLatencyMode mode) {
  std::lock_guard lock(_manager._mutex);
  if (std::ranges::find(_manager._support.LowLatencyModes, mode) ==
      _manager._support.LowLatencyModes.end())
    return FALSE;
  _newLowLatencyMode = mode;
  return TRUE;
}

BOOL PendingRenderSettingsChange::SetFrameGeneration(UINT32 generatedFrames) {
  std::lock_guard lock(_manager._mutex);
  if (generatedFrames > _manager._support.MaxGeneratedFrames) return FALSE;
  _newFrameGeneration = generatedFrames;
  return TRUE;
}

BOOL PendingRenderSettingsChange::SetRayTracing(BOOL enabled) {
  std::lock_guard lock(_manager._mutex);
  if (enabled && !_manager._support.RayTracing) return FALSE;
  _newRayTracing = enabled != FALSE;
  return TRUE;
}

BOOL PendingRenderSettingsChange::SetVR(BOOL enabled) {
  if (!_manager.GetVRSupport()) return FALSE;
  _newVR = enabled != FALSE;
  return TRUE;
}

void RenderSettingsManager::InitVR(bool moduleVR, bool saved,
                                   bool commandLine) {
  const bool support = moduleVR && OpenXR::Available("XR_KHR_D3D12_enable");
  std::lock_guard lock(_mutex);
  _vrSupport = support;
  _vrPreference = saved;
  if (support && (saved || commandLine)) _vrRequest = true;
}

BOOL RenderSettingsManager::GetVRSupport() {
  std::lock_guard lock(_mutex);
  return _vrSupport;
}

BOOL RenderSettingsManager::GetVR() {
  std::lock_guard lock(_mutex);
  return _vrActive;
}

std::optional<bool> RenderSettingsManager::TakeVRRequest() {
  std::lock_guard lock(_mutex);
  return std::exchange(_vrRequest, std::nullopt);
}

void RenderSettingsManager::SetVRState(bool active) {
  std::lock_guard lock(_mutex);
  _vrActive = active;
}

UINT64 RenderSettingsManager::GetUpscaleModeCount() {
  std::lock_guard lock(_mutex);
  return _support.UpscaleModes.size();
}

BOOL RenderSettingsManager::GetUpscaleMode(UINT64 index,
                                           MGDFUpscaleMode *mode) {
  std::lock_guard lock(_mutex);
  if (!mode || index >= _support.UpscaleModes.size()) return FALSE;
  *mode = _support.UpscaleModes[index];
  return TRUE;
}

UINT32 RenderSettingsManager::GetMaxGeneratedFrames() {
  std::lock_guard lock(_mutex);
  return _vrActive ? 0 : _support.MaxGeneratedFrames;
}

UINT64 RenderSettingsManager::GetLowLatencyModeCount() {
  std::lock_guard lock(_mutex);
  return _support.LowLatencyModes.size();
}

BOOL RenderSettingsManager::GetLowLatencyMode(UINT64 index,
                                              MGDFLowLatencyMode *mode) {
  std::lock_guard lock(_mutex);
  if (!mode || index >= _support.LowLatencyModes.size()) return FALSE;
  *mode = _support.LowLatencyModes[index];
  return TRUE;
}

MGDFLowLatencyMode RenderSettingsManager::GetCurrentLowLatencyMode() {
  std::lock_guard lock(_mutex);
  return _vrActive ? MGDF_LOW_LATENCY_OFF : _lowLatencyMode;
}

UINT32 RenderSettingsManager::GetFrameGeneration() {
  std::lock_guard lock(_mutex);
  return _vrActive ? 0 : _frameGeneration;
}

BOOL RenderSettingsManager::GetRayTracingSupport(
    MGDFRayTracingSupport *support) {
  std::lock_guard lock(_mutex);
  if (support) {
    support->ShaderExecutionReordering =
        _support.RayTracing && _support.ShaderExecutionReordering;
  }
  return _support.RayTracing;
}

BOOL RenderSettingsManager::GetRayTracing() {
  std::lock_guard lock(_mutex);
  return _rayTracing;
}

void RenderSettingsManager::SetGraphicsSupport(GraphicsSupport support) {
  _ASSERTE(!support.LowLatencyModes.empty() &&
           support.LowLatencyModes.front() == MGDF_LOW_LATENCY_OFF);
  std::lock_guard lock(_mutex);
  _support = std::move(support);
  ClampToSupportLocked();
}

void RenderSettingsManager::ClampToSupportLocked() {
  if (std::ranges::find(_support.LowLatencyModes, _lowLatencyMode) ==
      _support.LowLatencyModes.end())
    _lowLatencyMode = MGDF_LOW_LATENCY_OFF;
  _frameGeneration = (std::min)(_frameGeneration, _support.MaxGeneratedFrames);
  _rayTracing = _rayTracing && _support.RayTracing;
}

BOOL RenderSettingsManager::GetWindowAspectLock() {
  std::lock_guard lock(_mutex);
  return _windowAspectLock;
}

BOOL RenderSettingsManager::GetWindowResizeEnabled() {
  std::lock_guard lock(_mutex);
  return _windowResizeEnabled;
}

BOOL RenderSettingsManager::GetWindowSizeLimits(UINT32 *width, UINT32 *height) {
  if (!width || !height) return FALSE;
  RECT workArea{};
  if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0)) return FALSE;
  DWORD style = WS_OVERLAPPEDWINDOW;
  if (!GetWindowResizeEnabled())
    style &= ~(WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
  RECT frame{};
  if (!AdjustWindowRect(&frame, style, FALSE)) return FALSE;
  *width = static_cast<UINT32>((std::max)(1L, workArea.right - workArea.left -
                                                  (frame.right - frame.left)));
  *height = static_cast<UINT32>((std::max)(1L, workArea.bottom - workArea.top -
                                                   (frame.bottom - frame.top)));
  return TRUE;
}

RenderSettingsSnapshot RenderSettingsManager::TakeSnapshot() {
  std::lock_guard lock(_mutex);
  const bool hdr =
      _hdrEnabled && ((_fullScreen.FullScreen && _fullScreen.ExclusiveMode)
                          ? _primaryDisplayMode.SupportsHDR
                          : _currentOutputDisplayInfo.SupportsHDR);
  return {_generation,
          _fullScreen,
          std::exchange(_pendingWindowSize, std::nullopt),
          _primaryDisplayMode,
          _currentOutputDisplayInfo,
          hdr ? _hdrBackBufferFormat : _sdrBackBufferFormat,
          hdr ? _hdrColorSpace : DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709,
          _maxFrameLatency,
          _vrActive ? 0 : _frameGeneration};
}

void RenderSettingsManager::SetFullscreenFallback(
    const RenderSettingsSnapshot &snapshot,
    const MGDFFullScreenDesc &fullscreen) {
  std::lock_guard lock(_mutex);
  if (_generation == snapshot.Generation) _fullScreen = fullscreen;
}

void RenderSettingsManager::MarkChangeLocked() {
  ++_generation;
  _changePending.store(true);
}

void PendingRenderSettingsChange::Cancel() { _cancelled = true; }

RenderSettingsManager::RenderSettingsManager()
    : _sdrBackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
      _hdrBackBufferFormat(DXGI_FORMAT_R16G16B16A16_FLOAT),
      _hdrColorSpace(DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709),
      _hdrEnabled(false),
      _currentOutputDPI(96),
      _vsync(true),
      _screenX(0),
      _screenY(0),
      _primaryDisplayMode(MGDFDisplayMode{
          .Width = 0,
          .Height = 0,
          .RefreshRateNumerator = 0,
          .RefreshRateDenominator = 0,
          .SupportsHDR = false,
          .IsNativeSize = false,
      }),
      _currentOutputDisplayInfo(MGDFOutputDisplayInfo{
          .SupportsHDR = false,
          .MaxFullFrameLuminance = 80.0f,
          .MaxLuminance = 80.0f,
          .MinLuminance = 0.0001f,
          .SDRWhiteLevel = 1000,
          .Width = 0,
          .Height = 0,
      }),
      _fullScreen(MGDFFullScreenDesc{
          .FullScreen = false,
          .ExclusiveMode = false,
      }),
      _maxFrameLatency(1) {}

RenderSettingsManager::~RenderSettingsManager(void) {}

void RenderSettingsManager::CreatePendingSettingsChange(
    IMGDFPendingRenderSettingsChange **change) {
  ComObject<PendingRenderSettingsChange> p =
      MakeCom<PendingRenderSettingsChange>(*this);
  p.AddRawRef(change);
}

void RenderSettingsManager::GetCurrentOutputDisplayInfo(
    MGDFOutputDisplayInfo *info) {
  std::lock_guard<std::mutex> lock(_mutex);
  MGDFOutputDisplayInfo currentInfo = _currentOutputDisplayInfo;
  currentInfo.SupportsHDR = currentInfo.SupportsHDR && _hdrEnabled;
  *info = currentInfo;
}

DXGI_FORMAT RenderSettingsManager::GetSDRBackBufferFormat() {
  return _sdrBackBufferFormat;
}

DXGI_FORMAT RenderSettingsManager::GetHDRBackBufferFormat() {
  return _hdrBackBufferFormat;
}

void RenderSettingsManager::SetHDRFormat(MGDFHDRFormat format) {
  std::lock_guard lock(_mutex);
  const bool hdr10 = format == MGDF_HDR_FORMAT_HDR10;
  _hdrBackBufferFormat =
      hdr10 ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R16G16B16A16_FLOAT;
  _hdrColorSpace = hdr10 ? DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020
                         : DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
}

BOOL RenderSettingsManager::GetHDREnabled() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _hdrEnabled;
}

UINT RenderSettingsManager::GetCurrentOutputDPI() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _currentOutputDPI;
}

void RenderSettingsManager::SetOutputProperties(
    const MGDFOutputDisplayInfo &info, UINT currentDPI,
    const std::vector<MGDFDisplayMode> &primaryModes) {
  std::lock_guard<std::mutex> lock(_mutex);

  if (_currentOutputDisplayInfo.SDRWhiteLevel != info.SDRWhiteLevel) {
    MarkChangeLocked();
  }
  _currentOutputDisplayInfo = info;

  // update the fullscreen adaptor modes and current adaptor
  _primaryDisplayModes.clear();
  _primaryDisplayModes.insert(_primaryDisplayModes.end(), primaryModes.begin(),
                              primaryModes.end());
  for (const auto &mode : _primaryDisplayModes) {
    if (mode.Width == _primaryDisplayMode.Width &&
        mode.Height == _primaryDisplayMode.Height &&
        mode.RefreshRateNumerator == _primaryDisplayMode.RefreshRateNumerator &&
        mode.RefreshRateDenominator ==
            _primaryDisplayMode.RefreshRateDenominator) {
      if (_fullScreen.ExclusiveMode && _fullScreen.FullScreen &&
          mode.SupportsHDR != _primaryDisplayMode.SupportsHDR) {
        MarkChangeLocked();
      }
      _primaryDisplayMode = mode;
      break;
    }
  }

  if (_currentOutputDPI != currentDPI) {
    MarkChangeLocked();
  }
  _currentOutputDPI = currentDPI;
}

UINT32 RenderSettingsManager::GetMaxFrameLatency() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _maxFrameLatency;
}

BOOL RenderSettingsManager::GetVSync() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _vsync;
}

void RenderSettingsManager::GetFullscreen(MGDFFullScreenDesc *desc) {
  std::lock_guard<std::mutex> lock(_mutex);
  *desc = _fullScreen;
}

UINT64 RenderSettingsManager::GetPrimaryDisplayModeCount() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _primaryDisplayModes.size();
}

BOOL RenderSettingsManager::GetPrimaryDisplayMode(UINT64 index,
                                                  MGDFDisplayMode *mode) {
  if (!mode) return false;
  std::lock_guard<std::mutex> lock(_mutex);
  if (index < _primaryDisplayModes.size()) {
    auto m = _primaryDisplayModes.at(index);
    *mode = m;
    return true;
  }
  return false;
}

void RenderSettingsManager::GetPrimaryDisplayModes(MGDFDisplayMode **modes) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (modes && !_primaryDisplayModes.empty()) {
    memcpy_s(modes, _primaryDisplayModes.size() * sizeof(MGDFDisplayMode),
             _primaryDisplayModes.data(),
             _primaryDisplayModes.size() * sizeof(MGDFDisplayMode));
  }
}

BOOL RenderSettingsManager::GetPrimaryDisplayModeFromDimensions(
    UINT32 width, UINT32 height, MGDFDisplayMode *mode) {
  if (!mode) return false;
  std::lock_guard<std::mutex> lock(_mutex);

  MGDFDisplayMode bestMode{};
  bool foundMode = false;

  for (const auto &currentMode : _primaryDisplayModes) {
    if (currentMode.Width == width && currentMode.Height == height) {
      if (!foundMode ||
          // find the highest refresh rate for the given resolution
          ((static_cast<double>(currentMode.RefreshRateNumerator) /
                bestMode.RefreshRateDenominator >=
            static_cast<double>(bestMode.RefreshRateNumerator) /
                currentMode.RefreshRateDenominator) &&
           // and preferentially chose an HDR mode if HDR is enabled
           (!bestMode.SupportsHDR || currentMode.SupportsHDR))) {
        bestMode = currentMode;
        foundMode = true;
      }
    }
  }

  if (foundMode) {
    *mode = bestMode;
  }

  return foundMode;
}

BOOL RenderSettingsManager::GetNativePrimaryDisplayMode(MGDFDisplayMode *mode) {
  const INT32 nativeWidth = GetSystemMetrics(SM_CXSCREEN);
  const INT32 nativeHeight = GetSystemMetrics(SM_CYSCREEN);
  return GetPrimaryDisplayModeFromDimensions(nativeWidth, nativeHeight, mode);
}

MGDFDisplayMode *RenderSettingsManager::GetCurrentPrimaryDisplayMode(
    MGDFDisplayMode *mode) {
  std::lock_guard<std::mutex> lock(_mutex);
  *mode = _primaryDisplayMode;
  return mode;
}

UINT32 RenderSettingsManager::GetScreenX() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _screenX;
}

UINT32 RenderSettingsManager::GetScreenY() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _screenY;
}

bool RenderSettingsManager::IsBackBufferChangePending() {
  return _changePending.load();
}

void RenderSettingsManager::OnResize(UINT32 width, UINT32 height) {
  std::lock_guard<std::mutex> lock(_mutex);

  _screenX = width;
  _screenY = height;
}

void RenderSettingsManager::OnResetSwapChain(
    const RenderSettingsSnapshot &snapshot, DXGI_SWAP_CHAIN_DESC1 &desc,
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreenDesc,
    DXGI_COLOR_SPACE_TYPE &colorSpace, const RECT &windowSize) {
  std::lock_guard<std::mutex> lock(_mutex);

  fullscreenDesc.Windowed =
      !(snapshot.FullScreen.FullScreen && snapshot.FullScreen.ExclusiveMode);
  fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  fullscreenDesc.RefreshRate.Numerator =
      snapshot.PrimaryDisplayMode.RefreshRateNumerator;
  fullscreenDesc.RefreshRate.Denominator =
      snapshot.PrimaryDisplayMode.RefreshRateDenominator;

  desc.Width = snapshot.FullScreen.FullScreen
                   ? (snapshot.FullScreen.ExclusiveMode
                          ? snapshot.PrimaryDisplayMode.Width
                          : snapshot.Output.Width)
                   : windowSize.right;
  desc.Height = snapshot.FullScreen.FullScreen
                    ? (snapshot.FullScreen.ExclusiveMode
                           ? snapshot.PrimaryDisplayMode.Height
                           : snapshot.Output.Height)
                    : windowSize.bottom;
  desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  desc.BufferCount = 2;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  if (!snapshot.FullScreen.ExclusiveMode) {
    desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  }

  desc.Format = snapshot.Format;
  colorSpace = snapshot.ColorSpace;

  desc.SampleDesc = {1, 0};
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.Stereo = false;

  _screenX = desc.Width;
  _screenY = desc.Height;

  _changePending.store(_generation != snapshot.Generation);
}

void RenderSettingsManager::GetPreferences(IMGDFPreferenceSet **preferences) {
  std::lock_guard lock(_mutex);
  ComObject<PreferenceSetImpl> p = MakeCom<PreferenceSetImpl>();

  p->Preferences.insert(std::make_pair(PreferenceConstants::FULL_SCREEN,
                                       ToString(_fullScreen.FullScreen)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::FULL_SCREEN_EXCLUSIVE,
                     ToString(_fullScreen.ExclusiveMode)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::VSYNC, ToString(_vsync)));
  p->Preferences.insert(std::make_pair(PreferenceConstants::MAX_FRAME_LATENCY,
                                       ToString(_maxFrameLatency)));
  p->Preferences.insert(std::make_pair(PreferenceConstants::SCREEN_X,
                                       ToString(_primaryDisplayMode.Width)));
  p->Preferences.insert(std::make_pair(PreferenceConstants::SCREEN_Y,
                                       ToString(_primaryDisplayMode.Height)));

  p->Preferences.insert(
      std::make_pair(PreferenceConstants::HDR_ENABLED, ToString(_hdrEnabled)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::SDR_BACKBUFFER_FORMAT,
                     ToString(_sdrBackBufferFormat)));
  p->Preferences.insert(
      {PreferenceConstants::WINDOW_ASPECT_LOCK, ToString(_windowAspectLock)});
  p->Preferences.insert({PreferenceConstants::LOW_LATENCY_MODE,
                         ToString(static_cast<int>(_lowLatencyMode))});
  p->Preferences.insert(
      {PreferenceConstants::FRAME_GENERATION, ToString(_frameGeneration)});
  p->Preferences.insert(
      {PreferenceConstants::RAY_TRACING, ToString(_rayTracing)});
  p->Preferences.insert({PreferenceConstants::VR, ToString(_vrPreference)});
  p.AddRawRef(preferences);
}

void RenderSettingsManager::LoadPreferences(const ComObject<IMGDFGame> &game) {
  _ASSERTE(game);
  bool hasCurrentMode = false;

  bool savePreferences = false;
  std::string pref;
  if (GetPreference(game, PreferenceConstants::WINDOW_ASPECT_LOCK, pref)) {
    _windowAspectLock = FromString<bool>(pref);
  }
  if (GetPreference(game, PreferenceConstants::WINDOW_RESIZE, pref)) {
    _windowResizeEnabled = FromString<bool>(pref);
  }
  if (GetPreference(game, PreferenceConstants::FULL_SCREEN, pref)) {
    _fullScreen.FullScreen = FromString<bool>(pref);
  }

  if (GetPreference(game, PreferenceConstants::FULL_SCREEN_EXCLUSIVE, pref)) {
    _fullScreen.ExclusiveMode = FromString<bool>(pref);
  }

  if (GetPreference(game, PreferenceConstants::VSYNC, pref)) {
    _vsync = FromString<bool>(pref);
  }

  if (GetPreference(game, PreferenceConstants::MAX_FRAME_LATENCY, pref)) {
    _maxFrameLatency = FromString<UINT32>(pref);
  }

  if (GetPreference(game, PreferenceConstants::HDR_ENABLED, pref)) {
    _hdrEnabled = FromString<bool>(pref);
  }

  {
    std::lock_guard lock(_mutex);
    if (GetPreference(game, PreferenceConstants::LOW_LATENCY_MODE, pref)) {
      const int mode = FromString<int>(pref);
      _lowLatencyMode =
          mode >= MGDF_LOW_LATENCY_OFF && mode <= MGDF_LOW_LATENCY_BOOST
              ? static_cast<MGDFLowLatencyMode>(mode)
              : MGDF_LOW_LATENCY_ON;
    }
    if (GetPreference(game, PreferenceConstants::FRAME_GENERATION, pref)) {
      _frameGeneration = FromString<UINT32>(pref);
    }
    _rayTracing =
        !GetPreference(game, PreferenceConstants::RAY_TRACING, pref) ||
        FromString<bool>(pref);
    ClampToSupportLocked();
  }

  if (GetPreference(game, PreferenceConstants::SDR_BACKBUFFER_FORMAT, pref)) {
    _sdrBackBufferFormat = FromString<DXGI_FORMAT>(pref);
  }

  std::string xPref;
  std::string yPref;
  if (GetPreference(game, PreferenceConstants::SCREEN_X, xPref) &&
      GetPreference(game, PreferenceConstants::SCREEN_Y, yPref)) {
    hasCurrentMode = GetPrimaryDisplayModeFromDimensions(
        FromString<int>(xPref), FromString<int>(yPref), &_primaryDisplayMode);
  }

  if (!hasCurrentMode) {
    _primaryDisplayMode = GetBestAlternativeDisplayMode();

    LOG("No fullscreen resolution preferences found, using "
            << SCREEN_RES(_primaryDisplayMode),
        MGDF_LOG_LOW);
    game->SetPreference(PreferenceConstants::SCREEN_X,
                        ToString(_primaryDisplayMode.Width).c_str());
    game->SetPreference(PreferenceConstants::SCREEN_Y,
                        ToString(_primaryDisplayMode.Height).c_str());
    savePreferences = true;
  } else {
    LOG("Loaded fullscreen resolution preference for "
            << SCREEN_RES(_primaryDisplayMode),
        MGDF_LOG_LOW);
  }

  if (savePreferences) {
    game->SavePreferences();
  }
}

MGDFDisplayMode RenderSettingsManager::GetBestAlternativeDisplayMode() {
  // try to find the native resolution if possible
  MGDFDisplayMode mode;
  bool hasCurrentMode = GetNativePrimaryDisplayMode(&mode);
  if (!hasCurrentMode) {
    // set 1024*768 as the default
    hasCurrentMode = GetPrimaryDisplayModeFromDimensions(
        Resources::MIN_SCREEN_X, Resources::MIN_SCREEN_Y, &mode);
  }
  // or if 1024*768 was unavailble the first adaptor mode in the list
  if (!hasCurrentMode) {
    mode = _primaryDisplayModes.at(0);
  }
  return mode;
}

}  // namespace core
}  // namespace MGDF