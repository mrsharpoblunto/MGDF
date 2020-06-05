#pragma once

#include <d3d11.h>
#include <d3d11_1.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <MGDF/MGDFRenderSettingsManager.hpp>
#include <atomic>
#include <map>
#include <mutex>
#include <vector>

#include "MGDFGameImpl.hpp"

namespace MGDF {
namespace core {

// this class is accessed by the sim and render threads, so setting values and
// doing device resets must be synced up with a mutex
class RenderSettingsManager : public ComBase<IRenderSettingsManager> {
 public:
  RenderSettingsManager();
  virtual ~RenderSettingsManager();

  bool GetVSync() const final;
  void SetVSync(bool vsync) final;

  void GetFullscreen(FullScreenDesc *desc) const final;
  void SetFullscreen(const FullScreenDesc *desc) final;

  UINT32 GetMultiSampleLevelCount() const final;
  bool GetMultiSampleLevel(UINT32 index, UINT32 *level) const final;

  bool SetBackBufferMultiSampleLevel(UINT32 multisampleLevel) final;
  UINT32 GetBackBufferMultiSampleLevel() const final;
  bool SetCurrentMultiSampleLevel(UINT32 multisampleLevel) final;
  UINT32 GetCurrentMultiSampleLevel(UINT32 *quality) const final;

  UINT32 GetAdaptorModeCount() const final;
  bool GetAdaptorMode(UINT32 index, AdaptorMode *mode) const final;
  bool GetAdaptorMode(UINT32 width, UINT32 height,
                      AdaptorMode *mode) const final;
  void GetCurrentAdaptorMode(AdaptorMode *mode) const final;
  bool SetCurrentAdaptorMode(const AdaptorMode *mode) final;
  bool SetCurrentAdaptorModeToNative() final;
  void SetWindowSize(UINT32 width, UINT32 height) const final;

  UINT32 GetScreenX() const final;
  UINT32 GetScreenY() const final;
  void ApplySettings() final;

  void GetPreferences(IPreferenceSet **preferences) final;

  void LoadPreferences(const ComObject<IGame> &game);

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

  std::vector<AdaptorMode> _adaptorModes;
  AdaptorMode _currentAdaptorMode;

  std::vector<UINT32> _multiSampleLevels;
  std::map<UINT32, UINT32> _multiSampleQuality;
  UINT32 _currentMultiSampleLevel;
  UINT32 _backBufferMultiSampleLevel;

  UINT32 _screenX, _screenY;
  HWND _window;

  bool _vsync;
  FullScreenDesc _fullScreen;

  mutable std::mutex _mutex;
};

}  // namespace core
}  // namespace MGDF