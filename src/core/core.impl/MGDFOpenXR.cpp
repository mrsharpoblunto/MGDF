#include "StdAfx.h"

#include "MGDFOpenXR.hpp"

#include <algorithm>

#include "../common/MGDFLoggerImpl.hpp"

namespace MGDF::core {

OpenXRAction GetOpenXRAction(XrSessionState state) {
  switch (state) {
    case XR_SESSION_STATE_READY:
      return OpenXRAction::Start;
    case XR_SESSION_STATE_SYNCHRONIZED:
    case XR_SESSION_STATE_VISIBLE:
    case XR_SESSION_STATE_FOCUSED:
      return OpenXRAction::Run;
    case XR_SESSION_STATE_STOPPING:
      return OpenXRAction::EndSession;
    case XR_SESSION_STATE_LOSS_PENDING:
    case XR_SESSION_STATE_EXITING:
      return OpenXRAction::FallBack;
    default:
      return OpenXRAction::Wait;
  }
}

MGDFView ConvertOpenXRView(const XrView &view, UINT32 width, UINT32 height,
                           DXGI_FORMAT format) {
  return {{view.pose.position.x, view.pose.position.y, -view.pose.position.z},
          {-view.pose.orientation.x, -view.pose.orientation.y,
           view.pose.orientation.z, view.pose.orientation.w},
          view.fov.angleLeft,
          view.fov.angleRight,
          view.fov.angleUp,
          view.fov.angleDown,
          width,
          height,
          format};
}

DXGI_FORMAT SelectOpenXRFormat(
    const std::vector<int64_t> &formats,
    const std::function<bool(DXGI_FORMAT)> &canRender) {
  for (const auto preferred :
       {DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_B8G8R8A8_UNORM_SRGB}) {
    if (std::ranges::find(formats, static_cast<int64_t>(preferred)) !=
            formats.end() &&
        canRender(preferred))
      return preferred;
  }
  for (const auto value : formats) {
    const auto format = static_cast<DXGI_FORMAT>(value);
    if (canRender(format)) return format;
  }
  return DXGI_FORMAT_UNKNOWN;
}

OpenXR::~OpenXR() { Destroy(); }

bool OpenXR::Available(const char *extension) {
  uint32_t count = 0;
  if (XR_FAILED(
          xrEnumerateInstanceExtensionProperties(nullptr, 0, &count, nullptr)))
    return false;
  std::vector<XrExtensionProperties> properties(count,
                                                {XR_TYPE_EXTENSION_PROPERTIES});
  if (XR_FAILED(xrEnumerateInstanceExtensionProperties(nullptr, count, &count,
                                                       properties.data())))
    return false;
  return std::ranges::any_of(properties, [extension](const auto &property) {
    return strcmp(property.extensionName, extension) == 0;
  });
}

void OpenXR::Fail(const char *operation, XrResult result) {
  if (!_fallback) {
    LOG("OpenXR " << operation << " (" << result
                  << "); returning to the window",
        MGDF_LOG_LOW);
  }
  _fallback = true;
  _changed = true;
  _viewCount = 0;
}

bool OpenXR::Check(XrResult result, const char *operation) {
  if (XR_FAILED(result)) {
    Fail(operation, result);
    return false;
  }
  return true;
}

bool OpenXR::Prepare(const char *extension, const RequirementsQuery &query) {
  if (Prepared()) return !_fallback;
  _fallback = false;
  XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};
  strcpy_s(info.applicationInfo.applicationName, "MGDF");
  strcpy_s(info.applicationInfo.engineName, "MGDF");
  info.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
  info.enabledExtensionCount = 1;
  info.enabledExtensionNames = &extension;
  if (!Check(xrCreateInstance(&info, &_instance), "create instance"))
    return false;
  const XrSystemGetInfo system{XR_TYPE_SYSTEM_GET_INFO, nullptr,
                               XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
  if (!Check(xrGetSystem(_instance, &system, &_system), "get headset") ||
      !Check(query(_instance, _system, _requirements), "graphics requirements"))
    return false;
  uint32_t count = 0;
  if (!Check(xrEnumerateEnvironmentBlendModes(
                 _instance, _system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                 0, &count, nullptr),
             "get blend modes"))
    return false;
  if (!count) {
    Fail("no environment blend mode",
         XR_ERROR_ENVIRONMENT_BLEND_MODE_UNSUPPORTED);
    return false;
  }
  std::vector<XrEnvironmentBlendMode> modes(count);
  if (!Check(xrEnumerateEnvironmentBlendModes(
                 _instance, _system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                 count, &count, modes.data()),
             "get blend modes"))
    return false;
  _blendMode =
      std::ranges::find(modes, XR_ENVIRONMENT_BLEND_MODE_OPAQUE) != modes.end()
          ? XR_ENVIRONMENT_BLEND_MODE_OPAQUE
          : modes.front();
  return true;
}

bool OpenXR::CreateSession(const void *binding,
                           std::function<bool(DXGI_FORMAT)> canRender,
                           ImageQuery images) {
  if (_session) return !_fallback;
  _ASSERTE(_instance && !_fallback);
  const XrSessionCreateInfo info{XR_TYPE_SESSION_CREATE_INFO, binding, 0,
                                 _system};
  if (!Check(xrCreateSession(_instance, &info, &_session), "create session"))
    return false;
  const XrReferenceSpaceCreateInfo space{XR_TYPE_REFERENCE_SPACE_CREATE_INFO,
                                         nullptr,
                                         XR_REFERENCE_SPACE_TYPE_LOCAL,
                                         {{0, 0, 0, 1}, {0, 0, 0}}};
  if (!Check(xrCreateReferenceSpace(_session, &space, &_space),
             "create LOCAL space"))
    return false;
  _canRender = std::move(canRender);
  _images = std::move(images);
  _changed = true;
  return true;
}

bool OpenXR::RecreateSwapchains() {
  DestroySwapchains();
  if (!_session || _fallback) return false;
  uint32_t count = 0;
  if (!Check(xrEnumerateViewConfigurationViews(
                 _instance, _system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                 0, &count, nullptr),
             "get stereo views"))
    return false;
  if (count != 2) {
    Fail("requires two stereo views",
         XR_ERROR_VIEW_CONFIGURATION_TYPE_UNSUPPORTED);
    return false;
  }
  std::array<XrViewConfigurationView, 2> views{
      {{XR_TYPE_VIEW_CONFIGURATION_VIEW}, {XR_TYPE_VIEW_CONFIGURATION_VIEW}}};
  if (!Check(xrEnumerateViewConfigurationViews(
                 _instance, _system, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                 count, &count, views.data()),
             "get stereo views"))
    return false;
  if (!Check(xrEnumerateSwapchainFormats(_session, 0, &count, nullptr),
             "get formats"))
    return false;
  std::vector<int64_t> formats(count);
  if (!Check(
          xrEnumerateSwapchainFormats(_session, count, &count, formats.data()),
          "get formats"))
    return false;
  _format = SelectOpenXRFormat(formats, _canRender);
  if (_format == DXGI_FORMAT_UNKNOWN) {
    Fail("no renderable eye format", XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED);
    return false;
  }
  for (size_t i = 0; i < _eyes.size(); ++i) {
    auto &eye = _eyes[i];
    eye.Config = views[i];
    const XrSwapchainCreateInfo info{XR_TYPE_SWAPCHAIN_CREATE_INFO,
                                     nullptr,
                                     0,
                                     XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT |
                                         XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                                         XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT,
                                     _format,
                                     1,
                                     eye.Config.recommendedImageRectWidth,
                                     eye.Config.recommendedImageRectHeight,
                                     1,
                                     1,
                                     1};
    if (!Check(xrCreateSwapchain(_session, &info, &eye.Swapchain),
               "create eye swapchain") ||
        !Check(_images(eye.Swapchain, eye.Images), "get eye images"))
      return false;
    if (eye.Images.empty()) {
      Fail("empty eye swapchain", XR_ERROR_RUNTIME_FAILURE);
      return false;
    }
  }
  return true;
}

void OpenXR::PollEvents() {
  if (!_instance || _fallback) return;
  for (;;) {
    XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
    const auto result = xrPollEvent(_instance, &event);
    if (result == XR_EVENT_UNAVAILABLE) break;
    if (!Check(result, "poll events")) return;
    if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) {
      Fail("instance loss", XR_ERROR_INSTANCE_LOST);
      return;
    }
    if (event.type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) continue;
    const auto &change =
        reinterpret_cast<const XrEventDataSessionStateChanged &>(event);
    if (change.session != _session) continue;
    _state = change.state;
    switch (GetOpenXRAction(_state)) {
      case OpenXRAction::Start: {
        if (!_running) {
          const XrSessionBeginInfo info{
              XR_TYPE_SESSION_BEGIN_INFO, nullptr,
              XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};
          if (!Check(xrBeginSession(_session, &info), "begin session")) return;
          _running = true;
        }
        if (_stopRequested) RequestStop();
        break;
      }
      case OpenXRAction::EndSession:
        if (_running && !Check(xrEndSession(_session), "end session")) return;
        _running = false;
        _viewCount = 0;
        if (_stopRequested) {
          _fallback = true;
          _changed = true;
          return;
        }
        break;
      case OpenXRAction::FallBack:
        Fail("session ended", XR_SUCCESS);
        return;
      default:
        break;
    }
  }
}

void OpenXR::RequestStop() {
  _stopRequested = true;
  if (!_running) {
    _fallback = true;
    _changed = true;
  } else if (!_exitRequested) {
    _exitRequested = true;
    Check(xrRequestExitSession(_session), "request exit");
  }
}

bool OpenXR::BeginFrame() {
  _viewCount = 0;
  if (!_running || _fallback) return false;
  _frame = {XR_TYPE_FRAME_STATE};
  const XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};
  if (!Check(xrWaitFrame(_session, &wait, &_frame), "wait frame")) return false;
  const XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};
  if (!Check(xrBeginFrame(_session, &begin), "begin frame")) return false;
  _begun = true;
  if (!_frame.shouldRender || _stopRequested) return true;
  const XrViewLocateInfo locate{XR_TYPE_VIEW_LOCATE_INFO, nullptr,
                                XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                _frame.predictedDisplayTime, _space};
  XrViewState state{XR_TYPE_VIEW_STATE};
  uint32_t count = 0;
  if (!Check(xrLocateViews(_session, &locate, &state, 2, &count, _views.data()),
             "locate views"))
    return true;
  constexpr auto valid =
      XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;
  if (count != 2 || (state.viewStateFlags & valid) != valid) return true;
  for (auto &eye : _eyes) {
    const XrSwapchainImageAcquireInfo acquire{
        XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
    if (!Check(xrAcquireSwapchainImage(eye.Swapchain, &acquire, &eye.Image),
               "acquire eye"))
      return true;
    eye.Acquired = true;
    const XrSwapchainImageWaitInfo imageWait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
                                             nullptr, XR_INFINITE_DURATION};
    XrResult waited;
    do {
      waited = xrWaitSwapchainImage(eye.Swapchain, &imageWait);
    } while (waited == XR_TIMEOUT_EXPIRED);
    if (!Check(waited, "wait eye")) return true;
    eye.Waited = true;
    if (eye.Image >= eye.Images.size()) {
      Fail("invalid image index", XR_ERROR_RUNTIME_FAILURE);
      return true;
    }
  }
  _viewCount = 2;
  return true;
}

void OpenXR::ReleaseImages() {
  for (auto &eye : _eyes) {
    if (eye.Acquired && eye.Waited) {
      const XrSwapchainImageReleaseInfo release{
          XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
      Check(xrReleaseSwapchainImage(eye.Swapchain, &release), "release eye");
    }
    eye.Acquired = eye.Waited = false;
  }
}

void OpenXR::EndFrame(bool rendered) {
  if (!_begun) return;
  const bool submit = rendered && _viewCount == 2 && !_fallback;
  std::array<XrCompositionLayerProjectionView, 2> views{};
  for (size_t i = 0; i < views.size(); ++i) {
    const auto &eye = _eyes[i];
    views[i] = {
        XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
        nullptr,
        _views[i].pose,
        _views[i].fov,
        {eye.Swapchain,
         {{0, 0},
          {static_cast<int32_t>(eye.Config.recommendedImageRectWidth),
           static_cast<int32_t>(eye.Config.recommendedImageRectHeight)}},
         0}};
  }
  ReleaseImages();
  const XrCompositionLayerProjection projection{
      XR_TYPE_COMPOSITION_LAYER_PROJECTION,
      nullptr,
      0,
      _space,
      2,
      views.data()};
  const auto layer =
      reinterpret_cast<const XrCompositionLayerBaseHeader *>(&projection);
  const XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO,         nullptr,
                           _frame.predictedDisplayTime,    _blendMode,
                           submit && !_fallback ? 1U : 0U, &layer};
  Check(xrEndFrame(_session, &end), "end frame");
  _begun = false;
  _viewCount = 0;
}

void OpenXR::DestroySwapchains() {
  _ASSERTE(!_begun);
  for (auto &eye : _eyes) {
    eye.Images.clear();
    if (eye.Swapchain) xrDestroySwapchain(eye.Swapchain);
    eye = {};
  }
  _viewCount = 0;
}

void OpenXR::Destroy() {
  if (_begun) EndFrame(false);
  DestroySwapchains();
  if (_space) xrDestroySpace(_space);
  if (_session) xrDestroySession(_session);
  if (_instance) xrDestroyInstance(_instance);
  _space = XR_NULL_HANDLE;
  _session = XR_NULL_HANDLE;
  _instance = XR_NULL_HANDLE;
  _system = XR_NULL_SYSTEM_ID;
  _state = XR_SESSION_STATE_UNKNOWN;
  _running = _begun = _stopRequested = _exitRequested = _fallback = false;
  _canRender = {};
  _images = {};
  _changed = true;
}

bool OpenXR::TakeChange() { return std::exchange(_changed, false); }

MGDFView OpenXR::View(UINT32 index) const {
  _ASSERTE(index < _eyes.size());
  const auto &eye = _eyes[index];
  if (!eye.Swapchain) return {};
  return ConvertOpenXRView(_views[index], eye.Config.recommendedImageRectWidth,
                           eye.Config.recommendedImageRectHeight, _format);
}

ComObject<IUnknown> OpenXR::Target(UINT32 index) const {
  _ASSERTE(index < _viewCount);
  return _eyes[index].Images[_eyes[index].Image];
}

}  // namespace MGDF::core
