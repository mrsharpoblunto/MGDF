#include "StdAfx.h"

#include "MGDFRenderSettingsManagerImpl.hpp"

#include <sstream>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFPreferenceConstants.hpp"
#include "../common/MGDFPreferenceSet.hpp"
#include "../common/MGDFResources.hpp"

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
  if (_newBackBufferMultiSampleLevel.has_value()) {
    changed = true;
    _manager._backBufferMultiSampleLevel =
        _newBackBufferMultiSampleLevel.value();
  }
  if (_newCurrentMultiSampleLevel.has_value()) {
    changed = true;
    _manager._currentMultiSampleLevel = _newCurrentMultiSampleLevel.value();
  }
  if (_newSize.has_value()) {
    changed = true;
    _manager._screenX = _newSize.value().first;
    _manager._screenY = _newSize.value().second;
  }
  if (_newHDREnabled.has_value()) {
    changed = true;
    _manager._hdrEnabled = _newHDREnabled.value();
  }

  if (changed) {
    _manager._changePending.store(true);
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

BOOL PendingRenderSettingsChange::SetBackBufferMultiSampleLevel(
    UINT32 multisampleLevel) {
  std::lock_guard<std::mutex> lock(_manager._mutex);
  if (_manager._multiSampleQuality.find(multisampleLevel) !=
      _manager._multiSampleQuality.end()) {
    _newBackBufferMultiSampleLevel = multisampleLevel;
    return true;
  } else {
    return false;
  }
}

BOOL PendingRenderSettingsChange::SetCurrentMultiSampleLevel(
    UINT32 multisampleLevel) {
  std::lock_guard<std::mutex> lock(_manager._mutex);
  if (_manager._multiSampleQuality.find(multisampleLevel) !=
      _manager._multiSampleQuality.end()) {
    _newCurrentMultiSampleLevel = multisampleLevel;
    return true;
  } else {
    return false;
  }
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

void PendingRenderSettingsChange::Cancel() { _cancelled = true; }

RenderSettingsManager::RenderSettingsManager()
    : _currentMultiSampleLevel(1),
      _backBufferMultiSampleLevel(1),
      _sdrBackBufferFormat(DXGI_FORMAT_R8G8B8A8_UNORM),
      _hdrBackBufferFormat(DXGI_FORMAT_R16G16B16A16_FLOAT),
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

void RenderSettingsManager::InitFromDevice(
    const ComObject<ID3D11Device> &d3dDevice) {
  _ASSERTE(d3dDevice);

  std::lock_guard<std::mutex> lock(_mutex);
  Cleanup();

  // determine the supported multisampling settings for this device
  for (UINT32 i = 1; i < D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i) {
    UINT32 quality = 0;
    // TODO do we need to handle cases of support being different for SDR and
    // HDR render targets?
    if (FAILED(d3dDevice->CheckMultisampleQualityLevels(
            DXGI_FORMAT_R16G16B16A16_FLOAT, i, &quality)) ||
        quality == 0) {
      continue;
    }
    LOG("Found valid multisample level " << i, MGDF_LOG_MEDIUM);
    _multiSampleLevels.push_back(i);
    _multiSampleQuality[i] = quality;
  }
}

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
    _changePending.store(true);
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
        _changePending.store(true);
      }
      _primaryDisplayMode = mode;
      break;
    }
  }

  if (_currentOutputDPI != currentDPI) {
    _changePending.store(true);
  }
  _currentOutputDPI = currentDPI;
}

UINT64 RenderSettingsManager::GetMultiSampleLevelCount() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _multiSampleLevels.size();
}

BOOL RenderSettingsManager::GetMultiSampleLevel(UINT64 index, UINT32 *level) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!level) return false;
  if (index < _multiSampleLevels.size()) {
    *level = _multiSampleLevels[index];
    return true;
  }
  return false;
}

UINT32 RenderSettingsManager::GetCurrentMultiSampleLevel(UINT32 *quality) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (quality)
    *quality = _multiSampleQuality.find(_currentMultiSampleLevel)->second - 1;
  return _currentMultiSampleLevel;
}

UINT32 RenderSettingsManager::GetBackBufferMultiSampleLevel() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _backBufferMultiSampleLevel;
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
  bool exp = true;
  return _changePending.compare_exchange_weak(exp, true);
}

void RenderSettingsManager::OnResize(UINT32 width, UINT32 height) {
  std::lock_guard<std::mutex> lock(_mutex);

  _screenX = width;
  _screenY = height;
}

void RenderSettingsManager::OnResetSwapChain(
    DXGI_SWAP_CHAIN_DESC1 &desc,
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreenDesc, const RECT &windowSize) {
  std::lock_guard<std::mutex> lock(_mutex);

  fullscreenDesc.Windowed =
      !(_fullScreen.FullScreen && !_fullScreen.ExclusiveMode);
  fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  fullscreenDesc.RefreshRate.Numerator =
      _primaryDisplayMode.RefreshRateNumerator;
  fullscreenDesc.RefreshRate.Denominator =
      _primaryDisplayMode.RefreshRateDenominator;

  desc.Width = _fullScreen.FullScreen ? (_fullScreen.ExclusiveMode
                                             ? _primaryDisplayMode.Width
                                             : _currentOutputDisplayInfo.Width)
                                      : windowSize.right;
  desc.Height =
      _fullScreen.FullScreen
          ? (_fullScreen.ExclusiveMode ? _primaryDisplayMode.Height
                                       : _currentOutputDisplayInfo.Height)
          : windowSize.bottom;
  desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  desc.BufferCount = 2;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  if (!_fullScreen.ExclusiveMode) {
    desc.Flags |= DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
  }

  // fullscreen exclusive always uses the primary display output
  const bool supportsHDR =
      ((_fullScreen.FullScreen && _fullScreen.ExclusiveMode)
           ? _primaryDisplayMode.SupportsHDR
           : _currentOutputDisplayInfo.SupportsHDR) &&
      _hdrEnabled;

  if (supportsHDR) {
    desc.Format = _hdrBackBufferFormat;
  } else {
    desc.Format = _sdrBackBufferFormat;
  }

  desc.SampleDesc.Count = _backBufferMultiSampleLevel;
  desc.SampleDesc.Quality =
      _multiSampleQuality[_backBufferMultiSampleLevel] - 1;
  desc.Scaling = DXGI_SCALING_STRETCH;
  desc.Stereo = false;

  _screenX = desc.Width;
  _screenY = desc.Height;

  _changePending.store(false);
}

void RenderSettingsManager::Cleanup() {
  _changePending.store(false);
  _multiSampleLevels.clear();
  _multiSampleQuality.clear();
}

void RenderSettingsManager::GetPreferences(IMGDFPreferenceSet **preferences) {
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
      std::make_pair(PreferenceConstants::RT_MULTISAMPLE_LEVEL,
                     ToString(_currentMultiSampleLevel)));
  p->Preferences.insert(std::make_pair(PreferenceConstants::MULTISAMPLE_LEVEL,
                                       ToString(_backBufferMultiSampleLevel)));

  p->Preferences.insert(
      std::make_pair(PreferenceConstants::HDR_ENABLED, ToString(_hdrEnabled)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::SDR_BACKBUFFER_FORMAT,
                     ToString(_sdrBackBufferFormat)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::HDR_BACKBUFFER_FORMAT,
                     ToString(_hdrBackBufferFormat)));
  p.AddRawRef(preferences);
}

void RenderSettingsManager::LoadPreferences(const ComObject<IMGDFGame> &game) {
  _ASSERTE(game);
  bool hasCurrentMode = false;

  bool savePreferences = false;
  std::string pref;
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

  if (GetPreference(game, PreferenceConstants::SDR_BACKBUFFER_FORMAT, pref)) {
    _sdrBackBufferFormat = FromString<DXGI_FORMAT>(pref);
  }

  if (GetPreference(game, PreferenceConstants::HDR_BACKBUFFER_FORMAT, pref)) {
    _hdrBackBufferFormat = FromString<DXGI_FORMAT>(pref);
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

  // ensure the multisample level is not above what is supported.
  if (GetPreference(game, PreferenceConstants::RT_MULTISAMPLE_LEVEL, pref)) {
    _currentMultiSampleLevel = FromString<int>(pref);
  }

  if (_currentMultiSampleLevel >
      _multiSampleLevels.at(_multiSampleLevels.size() - 1)) {
    _currentMultiSampleLevel =
        _multiSampleLevels.at(_multiSampleLevels.size() - 1);
    LOG("RT multisample preference is not supported, using "
            << _currentMultiSampleLevel << " instead",
        MGDF_LOG_LOW);
    game->SetPreference(PreferenceConstants::RT_MULTISAMPLE_LEVEL,
                        ToString(_currentMultiSampleLevel).c_str());
    savePreferences = true;
  }

  if (GetPreference(game, PreferenceConstants::MULTISAMPLE_LEVEL, pref)) {
    _backBufferMultiSampleLevel = FromString<int>(pref);
  }
  if (_backBufferMultiSampleLevel >
      _multiSampleLevels.at(_multiSampleLevels.size() - 1)) {
    _backBufferMultiSampleLevel =
        _multiSampleLevels.at(_multiSampleLevels.size() - 1);
    LOG("Backbuffer multisample preference is not supported, using "
            << _backBufferMultiSampleLevel << " instead",
        MGDF_LOG_LOW);
    game->SetPreference(PreferenceConstants::MULTISAMPLE_LEVEL,
                        ToString(_backBufferMultiSampleLevel).c_str());
    savePreferences = true;
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