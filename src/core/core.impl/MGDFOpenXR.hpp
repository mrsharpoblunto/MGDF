#pragma once

#include <MGDF/MGDF.h>
#include <openxr/openxr.h>

#include <MGDF/ComObject.hpp>
#include <array>
#include <atomic>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace MGDF::core {

struct OpenXRGraphicsRequirements {
  LUID Adapter{};
  D3D_FEATURE_LEVEL MinFeatureLevel = D3D_FEATURE_LEVEL_11_0;
};

enum class OpenXRAction { Wait, Start, Run, EndSession, FallBack };
OpenXRAction GetOpenXRAction(XrSessionState state);
MGDFView ConvertOpenXRView(const XrView &view, UINT32 width, UINT32 height,
                           DXGI_FORMAT format);
DXGI_FORMAT SelectOpenXRFormat(
    const std::vector<int64_t> &formats,
    const std::function<bool(DXGI_FORMAT)> &canRender);

// All operations run on the render thread, or before that thread starts.
class OpenXR {
 public:
  using RequirementsQuery = std::function<XrResult(
      XrInstance, XrSystemId, OpenXRGraphicsRequirements &)>;
  using ImageQuery =
      std::function<XrResult(XrSwapchain, std::vector<ComObject<IUnknown>> &)>;
  OpenXR() = default;
  OpenXR(const OpenXR &) = delete;
  OpenXR &operator=(const OpenXR &) = delete;
  ~OpenXR();
  static bool Available(const char *extension);
  bool Prepare(const char *extension, const char *applicationName,
               const RequirementsQuery &query);
  bool CreateSession(const void *binding,
                     std::function<bool(DXGI_FORMAT, bool)> canRender,
                     ImageQuery images, std::function<void()> waitIdle);
  bool RecreateSwapchains();
  void PollEvents();
  void RequestStop();
  bool BeginFrame();
  void EndFrame(bool rendered);
  void Destroy();
  void Fail(const char *operation, XrResult result);

  bool Prepared() const { return _instance != XR_NULL_HANDLE; }
  bool HasSession() const { return _session != XR_NULL_HANDLE; }
  bool Running() const { return _running; }
  bool Focused() const {
    return _running && _state == XR_SESSION_STATE_FOCUSED;
  }
  bool NeedsFallback() const { return _fallback; }
  bool TakeChange();
  const OpenXRGraphicsRequirements &Requirements() const {
    return _requirements;
  }
  UINT32 ViewCount() const { return _viewCount; }
  MGDFView View(UINT32 index) const;
  ComObject<IUnknown> Target(UINT32 index) const;
  HRESULT GetView(UINT32 index, MGDFView *view) const;
  HRESULT GetTarget(UINT32 index, ComObject<IUnknown> &target) const;

  HRESULT GetViewMask(UINT32 index, UINT32 *vertexCount, FLOAT *xy,
                      UINT32 *indexCount, UINT32 *indices) const;
  HRESULT GetViewMaskVersion(UINT64 *version) const;
  HRESULT GetDepthTarget(UINT32 index, ComObject<IUnknown> &target) const;
  HRESULT SetViewDepthRange(UINT32 index, float nearZ, float farZ);
  HRESULT SetVRPanel(const MGDFVRPanel *panel);
  HRESULT GetPanelTarget(ComObject<IUnknown> &target) const;

 private:
  bool ExtensionEnabled(const char *name) const;
  HRESULT SessionStatus() const;
  bool Check(XrResult result, const char *operation);
  void DestroySwapchains();
  void ReleaseImages();
  struct ImageSet {
    XrSwapchain Swapchain = XR_NULL_HANDLE;
    std::vector<ComObject<IUnknown>> Images;
    uint32_t Image = 0;
    bool Acquired = false;
    bool Waited = false;
    UINT32 Width = 0;
    UINT32 Height = 0;
  };
  struct Eye : ImageSet {
    XrViewConfigurationView Config{XR_TYPE_VIEW_CONFIGURATION_VIEW};
    ImageSet Depth;
    bool DepthWritten = false;
    float Near = 0, Far = 0;
    std::vector<XrVector2f> MaskVertices;
    std::vector<UINT32> MaskIndices;
  };
  bool FeatureCheck(XrResult result, const char *operation, bool &logged);
  bool CreateImages(ImageSet &images, UINT32 width, UINT32 height,
                    DXGI_FORMAT format, bool depth, bool &logged);
  void DestroyImages(ImageSet &images);
  bool AcquireImages(ImageSet &images);
  void ReleaseImages(ImageSet &images);
  void RefreshMasks();
  void PreparePanel();
  XrInstance _instance = XR_NULL_HANDLE;
  XrSystemId _system = XR_NULL_SYSTEM_ID;
  XrSession _session = XR_NULL_HANDLE;
  XrSpace _space = XR_NULL_HANDLE;
  XrSpace _viewSpace = XR_NULL_HANDLE;
  XrSystemProperties _systemProperties{XR_TYPE_SYSTEM_PROPERTIES};
  XrSessionState _state = XR_SESSION_STATE_UNKNOWN;
  OpenXRGraphicsRequirements _requirements;
  std::array<Eye, 2> _eyes;
  std::array<XrView, 2> _views{{{XR_TYPE_VIEW}, {XR_TYPE_VIEW}}};
  DXGI_FORMAT _format = DXGI_FORMAT_UNKNOWN;
  XrEnvironmentBlendMode _blendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  XrFrameState _frame{XR_TYPE_FRAME_STATE};
  std::function<bool(DXGI_FORMAT, bool)> _canRender;
  std::function<void()> _waitIdle;
  ImageQuery _images;
  std::vector<std::string> _extensions;
  std::atomic<DWORD> _threadId{0};
  std::vector<const XrCompositionLayerBaseHeader *> _layers;
  DXGI_FORMAT _depthFormat = DXGI_FORMAT_UNKNOWN;
  HRESULT _depthStatus = E_NOTIMPL;
  HRESULT _maskStatus = E_NOTIMPL;
  HRESULT _panelStatus = S_FALSE;
  bool _depthErrorLogged = false;
  bool _maskErrorLogged = false;
  bool _panelErrorLogged = false;
  PFN_xrGetVisibilityMaskKHR _getMask = nullptr;
  UINT64 _maskVersion = 0;
  std::optional<MGDFVRPanel> _panel;
  std::optional<MGDFVRPanel> _framePanel;
  ImageSet _panelImages;
  UINT32 _viewCount = 0;
  bool _running = false;
  bool _begun = false;
  bool _stopRequested = false;
  bool _exitRequested = false;
  bool _fallback = false;
  bool _changed = false;
};

}  // namespace MGDF::core
