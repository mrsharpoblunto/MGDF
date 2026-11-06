#include "StdAfx.h"

#include "MGDFOpenXR.hpp"

#include <algorithm>
#include <cmath>
#include <string_view>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFVersionHelper.hpp"
#include "../common/MGDFVersionInfo.hpp"

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
  const auto pose =
      ConvertOpenXRPose(view.pose, XR_SPACE_LOCATION_POSITION_VALID_BIT |
                                       XR_SPACE_LOCATION_ORIENTATION_VALID_BIT);
  return {{pose.Position[0], pose.Position[1], pose.Position[2]},
          {pose.Orientation[0], pose.Orientation[1], pose.Orientation[2],
           pose.Orientation[3]},
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
  _inputFrame = {};
  _input->Publish(_inputFrame);
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

bool OpenXR::Prepare(const char *extension, const char *applicationName,
                     const RequirementsQuery &query) {
  if (Prepared()) return !_fallback;
  _threadId = GetCurrentThreadId();
  _fallback = false;
  XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};
  const std::string_view appName(applicationName);
  auto length = (std::min)(appName.size(),
                           sizeof(info.applicationInfo.applicationName) - 1);
  // A truncated name must end at a UTF-8 character boundary.
  while (length && length < appName.size() &&
         (static_cast<unsigned char>(appName[length]) & 0xC0) == 0x80)
    --length;
  memcpy(info.applicationInfo.applicationName, appName.data(), length);
  strcpy_s(info.applicationInfo.engineName, "MGDF");
  const auto version = VersionHelper::Create(MGDFVersionInfo::MGDF_VERSION());
  // Pack MGDF's four version components into OpenXR's 32-bit field.
  for (const auto component :
       {version.Major, version.Minor, version.Build, version.Revision}) {
    _ASSERTE(component >= -1 && component <= 255);
    info.applicationInfo.engineVersion =
        (info.applicationInfo.engineVersion << 8) |
        static_cast<uint32_t>((std::max)(0, component));
  }
  info.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
  uint32_t extensionCount = 0;
  if (!Check(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount,
                                                    nullptr),
             "list extensions"))
    return false;
  std::vector<XrExtensionProperties> properties(extensionCount,
                                                {XR_TYPE_EXTENSION_PROPERTIES});
  if (!Check(xrEnumerateInstanceExtensionProperties(
                 nullptr, extensionCount, &extensionCount, properties.data()),
             "list extensions"))
    return false;
  _extensions = {extension};
  for (const auto name : {XR_KHR_VISIBILITY_MASK_EXTENSION_NAME,
                          XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME,
                          XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME}) {
    if (std::ranges::any_of(properties, [name](const auto &property) {
          return strcmp(property.extensionName, name) == 0;
        }))
      _extensions.emplace_back(name);
  }
  std::vector<const char *> enabled;
  for (const auto &name : _extensions) enabled.push_back(name.c_str());
  info.enabledExtensionCount = static_cast<uint32_t>(enabled.size());
  info.enabledExtensionNames = enabled.data();
  if (!Check(xrCreateInstance(&info, &_instance), "create instance"))
    return false;
  const XrSystemGetInfo system{XR_TYPE_SYSTEM_GET_INFO, nullptr,
                               XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
  if (!Check(xrGetSystem(_instance, &system, &_system), "get headset") ||
      !Check(query(_instance, _system, _requirements), "graphics requirements"))
    return false;
  XrSystemEyeGazeInteractionPropertiesEXT gaze{
      XR_TYPE_SYSTEM_EYE_GAZE_INTERACTION_PROPERTIES_EXT};
  _systemProperties.next =
      ExtensionEnabled(XR_EXT_EYE_GAZE_INTERACTION_EXTENSION_NAME) ? &gaze
                                                                   : nullptr;
  if (!Check(xrGetSystemProperties(_instance, _system, &_systemProperties),
             "get system properties"))
    return false;
  _systemProperties.next = nullptr;
  _gazeSupported = gaze.supportsEyeGazeInteraction != XR_FALSE;
  if (ExtensionEnabled(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME)) {
    FeatureCheck(xrGetInstanceProcAddr(
                     _instance, "xrGetVisibilityMaskKHR",
                     reinterpret_cast<PFN_xrVoidFunction *>(&_getMask)),
                 "load hidden mask function", _maskErrorLogged);
  }
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
                           std::function<bool(DXGI_FORMAT, bool)> canRender,
                           ImageQuery images, std::function<void()> waitIdle) {
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
  auto viewSpace = space;
  viewSpace.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
  if (!Check(xrCreateReferenceSpace(_session, &viewSpace, &_viewSpace),
             "create VIEW space"))
    return false;
  _waitIdle = std::move(waitIdle);
  _canRender = std::move(canRender);
  _images = std::move(images);
  CreateActions();
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
  _format = SelectOpenXRFormat(
      formats, [this](auto format) { return _canRender(format, false); });
  if (_format == DXGI_FORMAT_UNKNOWN) {
    Fail("no renderable eye format", XR_ERROR_SWAPCHAIN_FORMAT_UNSUPPORTED);
    return false;
  }
  _depthFormat = DXGI_FORMAT_UNKNOWN;
  _depthStatus = E_NOTIMPL;
  if (ExtensionEnabled(XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME)) {
    for (const auto format :
         {DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_D24_UNORM_S8_UINT,
          DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_D32_FLOAT_S8X24_UINT}) {
      if (std::ranges::find(formats, static_cast<int64_t>(format)) !=
              formats.end() &&
          _canRender(format, true)) {
        _depthFormat = format;
        _depthStatus = S_OK;
        break;
      }
    }
  }
  bool colorError = false;
  for (size_t i = 0; i < _eyes.size(); ++i) {
    auto &eye = _eyes[i];
    eye.Config = views[i];
    if (!CreateImages(eye, eye.Config.recommendedImageRectWidth,
                      eye.Config.recommendedImageRectHeight, _format, false,
                      colorError)) {
      Fail("create eye images", XR_ERROR_RUNTIME_FAILURE);
      return false;
    }
    if (_depthStatus == S_OK &&
        !CreateImages(eye.Depth, eye.Width, eye.Height, _depthFormat, true,
                      _depthErrorLogged))
      _depthStatus = E_FAIL;
  }
  if (FAILED(_depthStatus)) {
    for (auto &eye : _eyes) DestroyImages(eye.Depth);
    _depthFormat = DXGI_FORMAT_UNKNOWN;
  }
  RefreshMasks();
  return true;
}

void OpenXR::PollEvents() {
  _threadId = GetCurrentThreadId();
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
    if (event.type == XR_TYPE_EVENT_DATA_VISIBILITY_MASK_CHANGED_KHR) {
      const auto &change =
          reinterpret_cast<const XrEventDataVisibilityMaskChangedKHR &>(event);
      if (change.session == _session &&
          change.viewConfigurationType ==
              XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
        RefreshMasks();
    }
    if (event.type != XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) continue;
    const auto &change =
        reinterpret_cast<const XrEventDataSessionStateChanged &>(event);
    if (change.session != _session) continue;
    _state = change.state;
    if (!Focused()) {
      _inputFrame = {};
      _inputFrame.ControllersStatus = S_FALSE;
      _inputFrame.GazeStatus = _gazeSupported ? S_FALSE : E_NOTIMPL;
      _input->Publish(_inputFrame);
    }
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
  _layers.clear();
  for (auto &eye : _eyes) eye.DepthWritten = false;
  _viewCount = 0;
  if (!_running || _fallback) return false;
  PreparePanel();
  _frame = {XR_TYPE_FRAME_STATE};
  const XrFrameWaitInfo wait{XR_TYPE_FRAME_WAIT_INFO};
  if (!Check(xrWaitFrame(_session, &wait, &_frame), "wait frame")) return false;
  const XrFrameBeginInfo begin{XR_TYPE_FRAME_BEGIN_INFO};
  if (!Check(xrBeginFrame(_session, &begin), "begin frame")) return false;
  _begun = true;
  UpdateInput();
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
    if (!AcquireImages(eye)) return true;
    if (_depthStatus == S_OK && !AcquireImages(eye.Depth)) return true;
  }
  if (_framePanel && !AcquireImages(_panelImages)) return true;
  _viewCount = 2;
  return true;
}

void OpenXR::ReleaseImages() {
  for (auto &eye : _eyes) {
    ReleaseImages(eye);
    ReleaseImages(eye.Depth);
  }
  ReleaseImages(_panelImages);
}

void OpenXR::EndFrame(bool rendered) {
  if (!_begun) return;
  const bool submit = rendered && _viewCount == 2 && !_fallback;
  std::array<XrCompositionLayerProjectionView, 2> views{};
  std::array<XrCompositionLayerDepthInfoKHR, 2> depths{};
  const bool depth =
      _depthStatus == S_OK && _eyes[0].DepthWritten && _eyes[1].DepthWritten;
  for (size_t i = 0; i < _eyes.size(); ++i) {
    const auto &eye = _eyes[i];
    const XrRect2Di rect{
        {0, 0},
        {static_cast<int32_t>(eye.Width), static_cast<int32_t>(eye.Height)}};
    depths[i] = {XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR,
                 nullptr,
                 {eye.Depth.Swapchain, rect, 0},
                 0,
                 1,
                 eye.Near,
                 eye.Far};
    views[i] = {XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW,
                depth ? &depths[i] : nullptr,
                _views[i].pose,
                _views[i].fov,
                {eye.Swapchain, rect, 0}};
  }
  ReleaseImages();
  const XrCompositionLayerProjection projection{
      XR_TYPE_COMPOSITION_LAYER_PROJECTION,
      nullptr,
      0,
      _space,
      2,
      views.data()};
  XrCompositionLayerQuad quad{XR_TYPE_COMPOSITION_LAYER_QUAD};
  _layers.clear();
  if (submit && !_fallback) {
    _layers.push_back(
        reinterpret_cast<const XrCompositionLayerBaseHeader *>(&projection));
    if (_framePanel) {
      const auto &p = *_framePanel;
      quad.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
      quad.space = p.Space == MGDF_VR_SPACE_VIEW ? _viewSpace : _space;
      quad.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
      quad.subImage = {_panelImages.Swapchain,
                       {{0, 0},
                        {static_cast<int32_t>(p.PixelWidth),
                         static_cast<int32_t>(p.PixelHeight)}},
                       0};
      quad.pose = {{-p.Orientation[0], -p.Orientation[1], p.Orientation[2],
                    p.Orientation[3]},
                   {p.Position[0], p.Position[1], -p.Position[2]}};
      quad.size = {p.Width, p.Height};
      _layers.push_back(
          reinterpret_cast<const XrCompositionLayerBaseHeader *>(&quad));
    }
  }
  const XrFrameEndInfo end{XR_TYPE_FRAME_END_INFO,
                           nullptr,
                           _frame.predictedDisplayTime,
                           _blendMode,
                           static_cast<uint32_t>(_layers.size()),
                           _layers.data()};
  Check(xrEndFrame(_session, &end), "end frame");
  _begun = false;
  _layers.clear();
  for (auto &eye : _eyes) eye.DepthWritten = false;
  _viewCount = 0;
}

void OpenXR::DestroySwapchains() {
  _ASSERTE(!_begun);
  for (auto &eye : _eyes) {
    DestroyImages(eye.Depth);
    DestroyImages(eye);
    eye = {};
  }
  DestroyImages(_panelImages);
  _framePanel.reset();
  _panelStatus = S_FALSE;
  _viewCount = 0;
}

void OpenXR::Destroy() {
  if (_begun) EndFrame(false);
  DestroySwapchains();
  DestroyActions();
  if (_space) xrDestroySpace(_space);
  if (_viewSpace) xrDestroySpace(_viewSpace);
  if (_session) xrDestroySession(_session);
  if (_instance) xrDestroyInstance(_instance);
  _space = _viewSpace = XR_NULL_HANDLE;
  _session = XR_NULL_HANDLE;
  _instance = XR_NULL_HANDLE;
  _system = XR_NULL_SYSTEM_ID;
  _state = XR_SESSION_STATE_UNKNOWN;
  _running = _begun = _stopRequested = _exitRequested = _fallback = false;
  _canRender = {};
  _images = {};
  _extensions.clear();
  _getMask = nullptr;
  _gazeSupported = false;
  _maskStatus = _depthStatus = E_NOTIMPL;
  _depthFormat = DXGI_FORMAT_UNKNOWN;
  _panel.reset();
  _depthErrorLogged = _maskErrorLogged = _panelErrorLogged = false;
  _waitIdle = {};
  _changed = true;
}

bool OpenXR::ExtensionEnabled(const char *name) const {
  return _instance && name &&
         std::ranges::find(_extensions, name) != _extensions.end();
}

HRESULT OpenXR::SessionStatus() const {
  if (_threadId != GetCurrentThreadId()) return E_ILLEGAL_METHOD_CALL;
  if (!_session) return E_ILLEGAL_METHOD_CALL;
  return _fallback ? E_FAIL : S_OK;
}

HRESULT OpenXR::GetView(UINT32 index, MGDFView *view, bool eyeFrame) const {
  if (!view) return E_INVALIDARG;
  *view = {};
  if (index >= _eyes.size()) return E_INVALIDARG;
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (eyeFrame && (!_begun || !_viewCount)) return E_ILLEGAL_METHOD_CALL;
  *view = View(index);
  if (!_begun || !_viewCount) {
    std::fill(std::begin(view->Position), std::end(view->Position), 0.0f);
    std::fill(std::begin(view->Orientation), std::end(view->Orientation), 0.0f);
    view->AngleLeft = view->AngleRight = view->AngleUp = view->AngleDown = 0;
  }
  return S_OK;
}

HRESULT OpenXR::GetTarget(UINT32 index, ComObject<IUnknown> &target) const {
  target.Clear();
  if (index >= _eyes.size()) return E_INVALIDARG;
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (!_begun || index >= _viewCount) return E_ILLEGAL_METHOD_CALL;
  const auto &eye = _eyes[index];
  _ASSERTE(eye.Waited && eye.Image < eye.Images.size());
  if (!eye.Waited || eye.Image >= eye.Images.size()) return E_FAIL;
  target = eye.Images[eye.Image];
  return S_OK;
}

bool OpenXR::TakeChange() { return std::exchange(_changed, false); }

MGDFView OpenXR::View(UINT32 index) const {
  if (index >= _eyes.size()) return {};
  _ASSERTE(index < _eyes.size());
  const auto &eye = _eyes[index];
  if (!eye.Swapchain) return {};
  auto view =
      ConvertOpenXRView(_views[index], eye.Config.recommendedImageRectWidth,
                        eye.Config.recommendedImageRectHeight, _format);
  view.DepthFormat = _depthFormat;
  return view;
}

ComObject<IUnknown> OpenXR::Target(UINT32 index) const {
  ComObject<IUnknown> target;
  GetTarget(index, target);
  return target;
}

bool OpenXR::FeatureCheck(XrResult result, const char *operation,
                          bool &logged) {
  if (XR_SUCCEEDED(result)) return true;
  if (!logged) {
    LOG("OpenXR " << operation << " (" << result << ")", MGDF_LOG_LOW);
    logged = true;
  }
  if (result == XR_ERROR_SESSION_LOST || result == XR_ERROR_INSTANCE_LOST)
    Fail(operation, result);
  return false;
}

bool OpenXR::CreateImages(ImageSet &images, UINT32 width, UINT32 height,
                          DXGI_FORMAT format, bool depth, bool &logged) {
  const XrSwapchainCreateInfo info{
      XR_TYPE_SWAPCHAIN_CREATE_INFO,
      nullptr,
      0,
      (depth ? XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
             : XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) |
          XR_SWAPCHAIN_USAGE_SAMPLED_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT,
      format,
      1,
      width,
      height,
      1,
      1,
      1};
  if (!FeatureCheck(xrCreateSwapchain(_session, &info, &images.Swapchain),
                    "create image swapchain", logged) ||
      !FeatureCheck(_images(images.Swapchain, images.Images),
                    "enumerate image swapchain", logged)) {
    DestroyImages(images);
    return false;
  }
  if (images.Images.empty()) {
    FeatureCheck(XR_ERROR_RUNTIME_FAILURE, "empty image swapchain", logged);
    DestroyImages(images);
    return false;
  }
  images.Width = width;
  images.Height = height;
  return true;
}

void OpenXR::DestroyImages(ImageSet &images) {
  images.Images.clear();
  if (images.Swapchain) xrDestroySwapchain(images.Swapchain);
  images = {};
}

bool OpenXR::AcquireImages(ImageSet &images) {
  const XrSwapchainImageAcquireInfo acquire{
      XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO};
  if (!Check(xrAcquireSwapchainImage(images.Swapchain, &acquire, &images.Image),
             "acquire image"))
    return false;
  images.Acquired = true;
  const XrSwapchainImageWaitInfo wait{XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO,
                                      nullptr, XR_INFINITE_DURATION};
  XrResult result;
  do {
    result = xrWaitSwapchainImage(images.Swapchain, &wait);
  } while (result == XR_TIMEOUT_EXPIRED);
  if (!Check(result, "wait image")) return false;
  images.Waited = true;
  if (images.Image >= images.Images.size()) {
    Fail("invalid image index", XR_ERROR_RUNTIME_FAILURE);
    return false;
  }
  return true;
}

void OpenXR::ReleaseImages(ImageSet &images) {
  if (images.Acquired && images.Waited) {
    const XrSwapchainImageReleaseInfo release{
        XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO};
    Check(xrReleaseSwapchainImage(images.Swapchain, &release), "release image");
  }
  images.Acquired = images.Waited = false;
}

void OpenXR::RefreshMasks() {
  static std::atomic<UINT64> revision{0};
  _maskVersion = ++revision;
  for (auto &eye : _eyes) {
    eye.MaskVertices.clear();
    eye.MaskIndices.clear();
  }
  _maskStatus = E_NOTIMPL;
  if (!_getMask) {
    if (ExtensionEnabled(XR_KHR_VISIBILITY_MASK_EXTENSION_NAME))
      _maskStatus = E_FAIL;
    return;
  }
  _maskStatus = S_OK;
  for (UINT32 i = 0; i < _eyes.size(); ++i) {
    auto &eye = _eyes[i];
    XrVisibilityMaskKHR mask{XR_TYPE_VISIBILITY_MASK_KHR};
    for (UINT32 attempt = 0; attempt < 3; ++attempt) {
      const auto result =
          _getMask(_session, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, i,
                   XR_VISIBILITY_MASK_TYPE_HIDDEN_TRIANGLE_MESH_KHR, &mask);
      if (result != XR_ERROR_SIZE_INSUFFICIENT &&
          !FeatureCheck(result, "get hidden area mask", _maskErrorLogged)) {
        _maskStatus = E_FAIL;
        break;
      }
      if (XR_SUCCEEDED(result) && mask.vertexCapacityInput &&
          mask.indexCapacityInput) {
        eye.MaskVertices.resize(mask.vertexCountOutput);
        eye.MaskIndices.resize(mask.indexCountOutput);
        break;
      }
      if (!mask.vertexCountOutput || !mask.indexCountOutput) break;
      if (attempt == 2) {
        FeatureCheck(XR_ERROR_SIZE_INSUFFICIENT, "unstable hidden area mask",
                     _maskErrorLogged);
        _maskStatus = E_FAIL;
        break;
      }
      eye.MaskVertices.resize(mask.vertexCountOutput);
      eye.MaskIndices.resize(mask.indexCountOutput);
      mask.vertexCapacityInput = mask.vertexCountOutput;
      mask.indexCapacityInput = mask.indexCountOutput;
      mask.vertices = eye.MaskVertices.data();
      mask.indices = eye.MaskIndices.data();
    }
    if (FAILED(_maskStatus)) {
      for (auto &item : _eyes) {
        item.MaskVertices.clear();
        item.MaskIndices.clear();
      }
      return;
    }
  }
}

HRESULT OpenXR::GetViewMask(UINT32 index, UINT32 *vertexCount, FLOAT *xy,
                            UINT32 *indexCount, UINT32 *indices) const {
  const auto vertices = vertexCount && xy ? *vertexCount : 0;
  const auto count = indexCount && indices ? *indexCount : 0;
  if (vertexCount) *vertexCount = 0;
  if (indexCount) *indexCount = 0;
  if (!vertexCount || !indexCount || index >= _eyes.size()) return E_INVALIDARG;
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (FAILED(_maskStatus)) return _maskStatus;
  const auto &eye = _eyes[index];
  *vertexCount = static_cast<UINT32>(eye.MaskVertices.size());
  *indexCount = static_cast<UINT32>(eye.MaskIndices.size());
  if ((xy && vertices < *vertexCount) || (indices && count < *indexCount))
    return E_INVALIDARG;
  if (!*vertexCount || !*indexCount) return S_FALSE;
  if (xy) {
    // Reflecting Z puts the mask at +1; its X/Y tangents and winding are
    // unchanged.
    for (UINT32 i = 0; i < *vertexCount; ++i) {
      xy[2 * i] = eye.MaskVertices[i].x;
      xy[2 * i + 1] = eye.MaskVertices[i].y;
    }
  }
  if (indices)
    std::copy(eye.MaskIndices.begin(), eye.MaskIndices.end(), indices);
  return S_OK;
}

HRESULT OpenXR::GetViewMaskVersion(UINT64 *version) const {
  if (!version) return E_INVALIDARG;
  *version = 0;
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (FAILED(_maskStatus)) return _maskStatus;
  *version = _maskVersion;
  return S_OK;
}

HRESULT OpenXR::GetDepthTarget(UINT32 index,
                               ComObject<IUnknown> &target) const {
  target.Clear();
  if (index >= _eyes.size()) return E_INVALIDARG;
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (FAILED(_depthStatus)) return _depthStatus;
  if (!_begun || index >= _viewCount) return E_ILLEGAL_METHOD_CALL;
  const auto &depth = _eyes[index].Depth;
  _ASSERTE(depth.Waited && depth.Image < depth.Images.size());
  if (!depth.Waited || depth.Image >= depth.Images.size()) return E_FAIL;
  target = depth.Images[depth.Image];
  return S_OK;
}

HRESULT OpenXR::SetViewDepthRange(UINT32 index, float nearZ, float farZ) {
  if (index >= _eyes.size() || !(nearZ > 0) || !(farZ > 0) || nearZ == farZ)
    return E_INVALIDARG;
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (FAILED(_depthStatus)) return _depthStatus;
  if (!_begun || index >= _viewCount) return E_ILLEGAL_METHOD_CALL;
  auto &eye = _eyes[index];
  eye.DepthWritten = true;
  eye.Near = nearZ;
  eye.Far = farZ;
  return S_OK;
}

HRESULT OpenXR::SetVRPanel(const MGDFVRPanel *panel) {
  if (panel) {
    float length = 0;
    for (const auto value : panel->Orientation) length += value * value;
    if (!panel->PixelWidth || !panel->PixelHeight ||
        !std::isfinite(panel->Width) || !(panel->Width > 0) ||
        !std::isfinite(panel->Height) || !(panel->Height > 0) ||
        !std::isfinite(length) || std::abs(length - 1) > 0.01f ||
        (panel->Space != MGDF_VR_SPACE_LOCAL &&
         panel->Space != MGDF_VR_SPACE_VIEW))
      return E_INVALIDARG;
    for (const auto value : panel->Position)
      if (!std::isfinite(value)) return E_INVALIDARG;
  }
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (_systemProperties.graphicsProperties.maxLayerCount < 2) return E_NOTIMPL;
  if (panel &&
      (panel->PixelWidth >
           _systemProperties.graphicsProperties.maxSwapchainImageWidth ||
       panel->PixelHeight >
           _systemProperties.graphicsProperties.maxSwapchainImageHeight))
    return E_INVALIDARG;
  if (panel) {
    if (!_panel || panel->PixelWidth != _panel->PixelWidth ||
        panel->PixelHeight != _panel->PixelHeight)
      _panelStatus = S_FALSE;
    _panel = *panel;
  } else {
    _panel.reset();
    _framePanel.reset();
    _panelStatus = S_FALSE;
  }
  return S_OK;
}

void OpenXR::PreparePanel() {
  _framePanel = _panel;
  if (!_framePanel) return;
  const auto &panel = *_framePanel;
  if (SUCCEEDED(_panelStatus) && (_panelImages.Width != panel.PixelWidth ||
                                  _panelImages.Height != panel.PixelHeight)) {
    if (_panelImages.Swapchain) _waitIdle();
    DestroyImages(_panelImages);
    _panelStatus =
        CreateImages(_panelImages, panel.PixelWidth, panel.PixelHeight, _format,
                     false, _panelErrorLogged)
            ? S_OK
            : E_FAIL;
  }
  if (FAILED(_panelStatus)) _framePanel.reset();
}

HRESULT OpenXR::GetPanelTarget(ComObject<IUnknown> &target) const {
  target.Clear();
  const auto status = SessionStatus();
  if (FAILED(status)) return status;
  if (_systemProperties.graphicsProperties.maxLayerCount < 2) return E_NOTIMPL;
  if (!_begun || !_viewCount) return E_ILLEGAL_METHOD_CALL;
  if (FAILED(_panelStatus)) return _panelStatus;
  if (!_framePanel) return S_FALSE;
  _ASSERTE(_panelImages.Waited &&
           _panelImages.Image < _panelImages.Images.size());
  if (!_panelImages.Waited || _panelImages.Image >= _panelImages.Images.size())
    return E_FAIL;
  target = _panelImages.Images[_panelImages.Image];
  return S_OK;
}

}  // namespace MGDF::core
