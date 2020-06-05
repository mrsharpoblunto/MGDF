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

#define BACKBUFFER_FORMAT DXGI_FORMAT_R8G8B8A8_UNORM
#define SCREEN_RES(mode)                  \
  mode.Width << "x" << mode.Height << "@" \
             << (mode.RefreshRateNumerator / mode.RefreshRateDenominator) + 1

namespace MGDF {
namespace core {

RenderSettingsManager::RenderSettingsManager()
    : _currentMultiSampleLevel(1),
      _backBufferMultiSampleLevel(1),
      _vsync(true),
      _screenX(0),
      _screenY(0),
      _window(NULL) {
  ZeroMemory(&_currentAdaptorMode, sizeof(AdaptorMode));
}

void RenderSettingsManager::InitFromDevice(
    HWND window, const ComObject<ID3D11Device> &d3dDevice,
    const ComObject<IDXGIAdapter1> &adapter) {
  _ASSERTE(d3dDevice);
  _ASSERTE(adapter);

  std::lock_guard<std::mutex> lock(_mutex);
  _window = window;
  Cleanup();

  ComObject<IDXGIOutput> temp;
  if (FAILED(adapter->EnumOutputs(0, temp.Assign()))) {
    return;
  }
  ComObject<IDXGIOutput1> output;
  if (FAILED(temp->QueryInterface<IDXGIOutput1>(output.Assign()))) {
    return;
  }

  UINT32 maxAdaptorModes = 0U;
  if (FAILED(output->GetDisplayModeList1(BACKBUFFER_FORMAT, 0, &maxAdaptorModes,
                                         nullptr))) {
    return;
  }

  std::vector<DXGI_MODE_DESC1> modes(maxAdaptorModes);
  if (FAILED(output->GetDisplayModeList1(BACKBUFFER_FORMAT, 0, &maxAdaptorModes,
                                         modes.data()))) {
    return;
  }

  bool foundMatchingCurrentAdaptor = false;

  for (UINT32 mode = 0; mode < maxAdaptorModes; ++mode) {
    const DXGI_MODE_DESC1 &displayMode = modes[mode];
    // Does this adaptor mode support  the desired format and is it above the
    // minimum required resolution
    if (displayMode.Format == BACKBUFFER_FORMAT &&
        displayMode.Scaling == DXGI_MODE_SCALING_UNSPECIFIED &&
        !displayMode.Stereo &&  // Stereo adapters not currently supported
        displayMode.Width >= Resources::MIN_SCREEN_X &&
        displayMode.Height >= Resources::MIN_SCREEN_Y) {
      AdaptorMode adaptorMode;
      adaptorMode.Width = displayMode.Width;
      adaptorMode.Height = displayMode.Height;
      adaptorMode.RefreshRateNumerator = displayMode.RefreshRate.Numerator;
      adaptorMode.RefreshRateDenominator = displayMode.RefreshRate.Denominator;
      _adaptorModes.push_back(adaptorMode);
      LOG("Found valid adapter mode " << SCREEN_RES(adaptorMode), LOG_MEDIUM);

      // try to preserve the current adaptor mode settings when devices change
      if (_currentAdaptorMode.Width == adaptorMode.Width &&
          _currentAdaptorMode.Height == adaptorMode.Height &&
          _currentAdaptorMode.RefreshRateNumerator ==
              adaptorMode.RefreshRateNumerator &&
          _currentAdaptorMode.RefreshRateDenominator ==
              adaptorMode.RefreshRateDenominator) {
        _currentAdaptorMode = adaptorMode;
        foundMatchingCurrentAdaptor = true;
      }
    }
  }

  // ensure we always have a current adaptor mode set
  if (!foundMatchingCurrentAdaptor) {
    _currentAdaptorMode = _adaptorModes.at(0);
  }

  // determine the supported multisampling settings for this device
  for (UINT32 i = 1; i < D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; ++i) {
    UINT32 quality = 0;
    if (FAILED(d3dDevice->CheckMultisampleQualityLevels(BACKBUFFER_FORMAT, i,
                                                        &quality)) ||
        quality == 0) {
      continue;
    }
    LOG("Found valid multisample level " << i, LOG_MEDIUM);
    _multiSampleLevels.push_back(i);
    _multiSampleQuality[i] = quality;
  }
}

RenderSettingsManager::~RenderSettingsManager(void) {}

UINT32 RenderSettingsManager::GetMultiSampleLevelCount() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return static_cast<UINT32>(_multiSampleLevels.size());
}

bool RenderSettingsManager::GetMultiSampleLevel(UINT32 index,
                                                UINT32 *level) const {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!level) return false;
  if (index < _multiSampleLevels.size()) {
    *level = _multiSampleLevels[index];
    return true;
  }
  return false;
}

bool RenderSettingsManager::SetCurrentMultiSampleLevel(
    UINT32 multisampleLevel) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_multiSampleQuality.find(multisampleLevel) != _multiSampleQuality.end()) {
    _currentMultiSampleLevel = multisampleLevel;
    return true;
  } else {
    return false;
  }
}

UINT32 RenderSettingsManager::GetCurrentMultiSampleLevel(
    UINT32 *quality) const {
  std::lock_guard<std::mutex> lock(_mutex);
  if (quality)
    *quality = _multiSampleQuality.find(_currentMultiSampleLevel)->second - 1;
  return _currentMultiSampleLevel;
}

bool RenderSettingsManager::SetBackBufferMultiSampleLevel(
    UINT32 multisampleLevel) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_multiSampleQuality.find(multisampleLevel) != _multiSampleQuality.end()) {
    _backBufferMultiSampleLevel = multisampleLevel;
    return true;
  } else {
    return false;
  }
}

UINT32 RenderSettingsManager::GetBackBufferMultiSampleLevel() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _backBufferMultiSampleLevel;
}

bool RenderSettingsManager::GetVSync() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _vsync;
}

void RenderSettingsManager::SetVSync(bool vsync) {
  std::lock_guard<std::mutex> lock(_mutex);
  _vsync = vsync;
}

void RenderSettingsManager::GetFullscreen(FullScreenDesc *desc) const {
  std::lock_guard<std::mutex> lock(_mutex);
  *desc = _fullScreen;
}

void RenderSettingsManager::SetFullscreen(const FullScreenDesc *desc) {
  std::lock_guard<std::mutex> lock(_mutex);
  _fullScreen = *desc;
}

UINT32 RenderSettingsManager::GetAdaptorModeCount() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return static_cast<UINT32>(_adaptorModes.size());
}

bool RenderSettingsManager::GetAdaptorMode(UINT32 index,
                                           AdaptorMode *mode) const {
  if (!mode) return false;
  std::lock_guard<std::mutex> lock(_mutex);
  if (index < _adaptorModes.size()) {
    auto m = _adaptorModes.at(index);
    mode->Width = m.Width;
    mode->Height = m.Height;
    mode->RefreshRateNumerator = m.RefreshRateNumerator;
    mode->RefreshRateDenominator = m.RefreshRateDenominator;
    return true;
  }
  return false;
}

bool RenderSettingsManager::GetAdaptorMode(UINT32 width, UINT32 height,
                                           AdaptorMode *mode) const {
  if (!mode) return false;
  std::lock_guard<std::mutex> lock(_mutex);
  bool result = false;
  for (auto currentMode : _adaptorModes) {
    if (currentMode.Width == width && currentMode.Height == height) {
      if (!result) {
        mode->Width = currentMode.Width;
        mode->Height = currentMode.Height;
        mode->RefreshRateNumerator = currentMode.RefreshRateNumerator;
        mode->RefreshRateDenominator = currentMode.RefreshRateDenominator;
        result = true;
      } else if (currentMode.RefreshRateNumerator >=
                     mode->RefreshRateNumerator &&
                 currentMode.RefreshRateDenominator <=
                     mode->RefreshRateDenominator) {
        mode->RefreshRateNumerator = currentMode.RefreshRateNumerator;
        mode->RefreshRateDenominator = currentMode.RefreshRateDenominator;
      }
    }
  }
  return result;
}

void RenderSettingsManager::GetCurrentAdaptorMode(AdaptorMode *mode) const {
  if (!mode) return;
  std::lock_guard<std::mutex> lock(_mutex);
  mode->Width = _currentAdaptorMode.Width;
  mode->Height = _currentAdaptorMode.Height;
  mode->RefreshRateNumerator = _currentAdaptorMode.RefreshRateNumerator;
  mode->RefreshRateDenominator = _currentAdaptorMode.RefreshRateDenominator;
}

UINT32 RenderSettingsManager::GetScreenX() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _screenX;
}

UINT32 RenderSettingsManager::GetScreenY() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _screenY;
}

bool RenderSettingsManager::SetCurrentAdaptorMode(const AdaptorMode *mode) {
  if (!mode) return false;
  std::lock_guard<std::mutex> lock(_mutex);

  for (auto currentMode : _adaptorModes) {
    if (currentMode.Width == mode->Width &&
        currentMode.Height == mode->Height &&
        currentMode.RefreshRateDenominator == mode->RefreshRateDenominator &&
        currentMode.RefreshRateNumerator == mode->RefreshRateNumerator) {
      _currentAdaptorMode = currentMode;
      return true;
    }
  }

  return false;
}

bool RenderSettingsManager::SetCurrentAdaptorModeToNative() {
  const INT32 nativeWidth = GetSystemMetrics(SM_CXSCREEN);
  const INT32 nativeHeight = GetSystemMetrics(SM_CYSCREEN);
  AdaptorMode mode;
  if (GetAdaptorMode(nativeWidth, nativeHeight, &mode)) {
    _currentAdaptorMode = mode;
    LOG("Set adaptor mode to native resolution "
            << SCREEN_RES(_currentAdaptorMode),
        LOG_LOW);
    return true;
  }
  return false;
}

void RenderSettingsManager::SetWindowSize(UINT32 width, UINT32 height) const {
  if (!_fullScreen.FullScreen && _window) {
    SetWindowPos(_window, 0, 0, 0, width, height,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

void RenderSettingsManager::ApplySettings() {
  std::lock_guard<std::mutex> lock(_mutex);
  _changePending.store(true);
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
      !_fullScreen.FullScreen || !_fullScreen.ExclusiveMode;
  fullscreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  fullscreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  fullscreenDesc.RefreshRate.Numerator =
      _currentAdaptorMode.RefreshRateNumerator;
  fullscreenDesc.RefreshRate.Denominator =
      _currentAdaptorMode.RefreshRateDenominator;

  desc.Width =
      !_fullScreen.FullScreen ? windowSize.right : _currentAdaptorMode.Width;
  desc.Height =
      !_fullScreen.FullScreen ? windowSize.bottom : _currentAdaptorMode.Height;
  desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  desc.BufferCount = 2;
  desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
  desc.Format = BACKBUFFER_FORMAT;
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
  _adaptorModes.clear();
  _multiSampleLevels.clear();
  _multiSampleQuality.clear();
}

void RenderSettingsManager::GetPreferences(IPreferenceSet **preferences) {
  ComObject<PreferenceSetImpl> p(new PreferenceSetImpl());

  p->Preferences.insert(std::make_pair(PreferenceConstants::FULL_SCREEN,
                                       ToString(_fullScreen.FullScreen)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::FULL_SCREEN_EXCLUSIVE,
                     ToString(_fullScreen.ExclusiveMode)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::VSYNC, ToString(_vsync)));
  p->Preferences.insert(std::make_pair(PreferenceConstants::SCREEN_X,
                                       ToString(_currentAdaptorMode.Width)));
  p->Preferences.insert(std::make_pair(PreferenceConstants::SCREEN_Y,
                                       ToString(_currentAdaptorMode.Height)));
  p->Preferences.insert(
      std::make_pair(PreferenceConstants::RT_MULTISAMPLE_LEVEL,
                     ToString(_currentMultiSampleLevel)));
  p->Preferences.insert(std::make_pair(PreferenceConstants::MULTISAMPLE_LEVEL,
                                       ToString(_backBufferMultiSampleLevel)));

  p.AddRawRef(preferences);
}

void RenderSettingsManager::LoadPreferences(const ComObject<IGame> &game) {
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

  std::string xPref;
  std::string yPref;
  if (GetPreference(game, PreferenceConstants::SCREEN_X, xPref) &&
      GetPreference(game, PreferenceConstants::SCREEN_Y, yPref)) {
    hasCurrentMode = GetAdaptorMode(
        FromString<int>(xPref), FromString<int>(yPref), &_currentAdaptorMode);
  }

  if (!hasCurrentMode) {
    // set 1024*768 as the default
    hasCurrentMode = GetAdaptorMode(
        Resources::MIN_SCREEN_X, Resources::MIN_SCREEN_Y, &_currentAdaptorMode);
    // or if 1024*768 was unavailble the first adaptor mode in the list
    if (!hasCurrentMode) {
      _currentAdaptorMode = _adaptorModes.at(0);
    }

    // try to find the native resolution if possible, otherwise stick to the
    // default found above if none are found.
    SetCurrentAdaptorModeToNative();

    LOG("No screen resolution preferences found, using "
            << SCREEN_RES(_currentAdaptorMode),
        LOG_LOW);
    game->SetPreference(PreferenceConstants::SCREEN_X,
                        ToString(_currentAdaptorMode.Width).c_str());
    game->SetPreference(PreferenceConstants::SCREEN_Y,
                        ToString(_currentAdaptorMode.Height).c_str());
    savePreferences = true;
  } else {
    LOG("Loaded screen resolution preference for "
            << SCREEN_RES(_currentAdaptorMode),
        LOG_LOW);
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
        LOG_LOW);
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
        LOG_LOW);
    game->SetPreference(PreferenceConstants::MULTISAMPLE_LEVEL,
                        ToString(_backBufferMultiSampleLevel).c_str());
    savePreferences = true;
  }

  if (savePreferences) {
    game->SavePreferences();
  }
}

}  // namespace core
}  // namespace MGDF