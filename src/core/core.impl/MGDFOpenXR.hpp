#pragma once

#include <MGDF/MGDF.h>
#include <openxr/openxr.h>

#include <MGDF/ComObject.hpp>
#include <array>
#include <deque>
#include <functional>
#include <span>
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
               const RequirementsQuery &query,
               std::span<const char *const> extensions = {});
  bool CreateSession(const void *binding,
                     std::function<bool(DXGI_FORMAT)> canRender,
                     ImageQuery images);
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
  MGDFOpenXRState State() const;
  bool ExtensionEnabled(const char *name) const;
  bool PollEvent(void *buffer, UINT32 size);
  HRESULT AddCompositionLayers(UINT32 count, const void *const *layers);
  HRESULT SetProjectionViewNext(UINT32 index, const void *next);

 private:
  bool Check(XrResult result, const char *operation);
  void DestroySwapchains();
  void ReleaseImages();
  struct Eye {
    XrSwapchain Swapchain = XR_NULL_HANDLE;
    XrViewConfigurationView Config{XR_TYPE_VIEW_CONFIGURATION_VIEW};
    std::vector<ComObject<IUnknown>> Images;
    uint32_t Image = 0;
    bool Acquired = false;
    bool Waited = false;
  };
  XrInstance _instance = XR_NULL_HANDLE;
  XrSystemId _system = XR_NULL_SYSTEM_ID;
  XrSession _session = XR_NULL_HANDLE;
  XrSpace _space = XR_NULL_HANDLE;
  XrSessionState _state = XR_SESSION_STATE_UNKNOWN;
  OpenXRGraphicsRequirements _requirements;
  std::array<Eye, 2> _eyes;
  std::array<XrView, 2> _views{{{XR_TYPE_VIEW}, {XR_TYPE_VIEW}}};
  DXGI_FORMAT _format = DXGI_FORMAT_UNKNOWN;
  XrEnvironmentBlendMode _blendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  XrFrameState _frame{XR_TYPE_FRAME_STATE};
  std::function<bool(DXGI_FORMAT)> _canRender;
  ImageQuery _images;
  std::vector<std::string> _extensions;
  std::deque<XrEventDataBuffer> _events;
  bool _eventOverflowLogged = false;
  std::vector<const XrCompositionLayerBaseHeader *> _layers;
  std::array<const void *, 2> _viewNext{};
  UINT32 _viewCount = 0;
  bool _running = false;
  bool _begun = false;
  bool _stopRequested = false;
  bool _exitRequested = false;
  bool _fallback = false;
  bool _changed = false;
};

}  // namespace MGDF::core
