#pragma once

#include <d3d11.h>
#include <dxgi1_6.h>

#include <MGDF/ComObject.hpp>
#include <functional>
#include <vector>

#include "MGDFRenderBackend.hpp"

namespace MGDF {
namespace core {

class D3D11RenderBackend : public IRenderBackend {
 public:
  D3D11RenderBackend(
      const ComObject<IDXGIFactory6> &factory,
      std::vector<D3D_FEATURE_LEVEL> levels,
      std::function<void(const char *, const char *)> fatalError);

  bool RTInit() final;
  bool RTIsInitialized() const final;
  void RTUninit(bool exclusiveMode) final;
  void RTCreateSwapChain(HWND window, const DXGI_SWAP_CHAIN_DESC1 &desc,
                         std::optional<UINT> maxFrameLatency) final;
  HRESULT RTResizeBackBuffer(const DXGI_SWAP_CHAIN_DESC1 &desc) final;
  void RTSetExclusiveFullscreen() final;
  void RTSetWindowed() final;
  bool RTWaitForFrame() final;
  void RTWaitForGpuIdle() final;
  void RTClear() final;
  HRESULT RTPresent(UINT syncInterval, UINT flags) final;
  HRESULT RTGetDeviceRemovedReason() const final;

  ComObject<IUnknown> RTGetDevice() final;
  ComObject<IUnknown> RTGetBackBuffer() final;
  ComObject<IUnknown> RTGetDepthStencilBuffer() final;
  ComObject<IDXGIAdapter> RTGetAdapter() final;
  MGDFBackBufferInfo RTGetBackBufferInfo() const final;

 private:
  void RTClearBackBuffer();
  void RTLogDebugMessages();
  void FatalError(const char *sender, const char *message);

  // The framework owns and refreshes the shared factory.
  const ComObject<IDXGIFactory6> &_rtFactory;
  std::vector<D3D_FEATURE_LEVEL> _rtLevels;
  std::function<void(const char *, const char *)> _fatalError;
  ComObject<ID3D11Device> _rtD3dDevice;
  ComObject<ID3D11DeviceContext> _rtImmediateContext;
  ComObject<IDXGISwapChain1> _rtSwapChain;
  ComObject<ID3D11RenderTargetView> _rtRenderTargetView;
  ComObject<ID3D11DepthStencilView> _rtDepthStencilView;
  ComObject<ID3D11Texture2D> _rtDepthStencilBuffer;
  ComObject<ID3D11Texture2D> _rtBackBuffer;
  DXGI_SWAP_CHAIN_DESC1 _rtSwapDesc{};
  HANDLE _rtFrameWaitableObject;
};

}  // namespace core
}  // namespace MGDF
