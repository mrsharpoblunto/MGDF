#pragma once

#include <MGDF/MGDF.h>
#include <dxgi1_6.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>

namespace MGDF {
namespace core {

struct StreamlineState;

struct StreamlineFrameStats {
  bool HasLatency = false;
  double RenderLatency = 0.0;
  UINT64 PresentedFrames = 0;
};

/**
Owns NVIDIA Streamline for the D3D12 backend in manual hooking mode. Every
method is safe when Streamline is not loaded: queries report no support, frame
calls do nothing and Upscale returns E_NOTIMPL. Methods with the RT prefix run
on the render thread; the others may run on any thread.
*/
class Streamline {
 public:
  Streamline(UINT32 requestedFeatures, std::wstring pluginDirectory);
  ~Streamline();
  Streamline(const Streamline &) = delete;
  Streamline &operator=(const Streamline &) = delete;

  // Call before the backend creates queues or swap chains. Logs the reason
  // once and stays inactive if Streamline is not usable on this adapter.
  void RTInit(ID3D12Device *device, IDXGIAdapter1 *adapter);
  // Release every proxy first: proxies call into Streamline when released.
  void RTShutdown();
  bool RTIsActive() const;

  // Null while inactive. Create queues through it: Streamline needs to see
  // them, and it returns proxy queues.
  ComObject<ID3D12Device> RTGetProxyDevice() const;
  // Swap chains must come from the proxy factory so that Streamline sees
  // every present.
  ComObject<IDXGIFactory2> RTUpgradeFactory(IDXGIFactory2 *factory);
  template <typename T>
  ComObject<T> RTGetNative(IUnknown *proxy) {
    ComObject<IUnknown> native = RTGetNativeUnknown(proxy);
    return native ? native.As<T>() : ComObject<T>();
  }

  MGDFGraphicsFeatureSupport GetSupport() const;
  HRESULT GetUpscaleRenderSize(MGDFUpscaleMode mode, UINT32 displayWidth,
                               UINT32 displayHeight,
                               MGDFUpscaleRenderSize *size) const;
  UINT GetLatencyPingMessage() const;
  void QueueLatencyPing();
  StreamlineFrameStats GetFrameStats() const;

  // Selects frame generation for the next swap chain. Call with no swap chain
  // alive. Returns true if frame generation runs on the new swap chain.
  bool RTPrepareSwapChain(UINT32 generatedFrames, DXGI_FORMAT format);
  // Call before ResizeBuffers, SetFullscreenState or swap chain release.
  void RTSuspendFrameGeneration();
  void RTResumeFrameGeneration();
  UINT RTGetSyncInterval(UINT requested) const;

  void RTBeginFrame(MGDFLowLatencyMode lowLatency);
  void RTSetFrameMarker(MGDFFrameMarker marker);
  void RTSetFrameConstants(const MGDFFrameConstants *constants);
  HRESULT RTUpscale(ID3D12GraphicsCommandList *list,
                    const MGDFUpscaleDesc *desc);
  HRESULT RTSetFrameGenerationInputs(const MGDFFrameGenerationDesc *desc);
  void RTBeforePresent();
  void RTAfterPresent();

 private:
  ComObject<IUnknown> RTGetNativeUnknown(IUnknown *proxy);
  bool LoadInterposer();
  void ShutdownLocked();
  void ClearFrameGenerationTagsLocked();
  void LogOnce(const std::string &key, const std::string &message,
               MGDFLogLevel level) const;

  const UINT32 _requested;
  const std::wstring _directory;
  HMODULE _interposer = nullptr;
  std::unique_ptr<StreamlineState> _state;
  std::atomic<UINT> _pingMessage = 0;
  std::atomic<bool> _ping = false;
  mutable std::mutex _mutex;
};

}  // namespace core
}  // namespace MGDF
