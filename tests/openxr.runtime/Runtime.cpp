#include <MGDF/MGDF.h>
#include <dxgi1_6.h>

#include <MGDF/ComObject.hpp>

#define XR_USE_PLATFORM_WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_GRAPHICS_API_D3D12
#include <openxr/openxr_platform.h>

#include <array>
#include <cstring>
#include <deque>
#include <vector>

#include "Control.hpp"

using namespace MGDF;

struct XrInstance_T {};
struct XrSession_T {
  ComObject<ID3D11Device> Device11;
  ComObject<ID3D12Device> Device12;
  ComObject<ID3D12CommandQueue> Queue;
  bool Running = false;
  bool Waited = false;
  bool Begun = false;
  XrSessionState State = XR_SESSION_STATE_IDLE;
};
struct XrSpace_T {
  XrSession Session;
};
struct XrSwapchain_T {
  XrSwapchainCreateInfo Info{};
  std::vector<ComObject<ID3D11Texture2D>> Images11;
  std::vector<ComObject<ID3D12Resource>> Images12;
  UINT32 Index = 2;
  bool Acquired = false;
  bool Waited = false;
  bool Released = false;
};

namespace {
FakeXRConfig Config;
FakeXRStats Stats;
XrSession Session = XR_NULL_HANDLE;
struct Event {
  XrSessionState State = XR_SESSION_STATE_UNKNOWN;
  UINT32 Marker = 0;
};
std::deque<Event> Events;
// Keep retired handle storage until test reset so recreation cannot reuse a
// numeric handle and hide stale state returned to the module.
std::vector<std::unique_ptr<XrInstance_T>> RetiredInstances;
std::vector<std::unique_ptr<XrSession_T>> RetiredSessions;
std::vector<std::unique_ptr<XrSpace_T>> RetiredSpaces;
bool InstanceLost = false;

XrResult Reject() {
  ++Stats.Errors;
  return XR_ERROR_CALL_ORDER_INVALID;
}

template <typename T, size_t N>
XrResult Enumerate(const std::array<T, N> &values, uint32_t capacity,
                   uint32_t *count, T *output) {
  *count = static_cast<uint32_t>(N);
  if (!capacity) return XR_SUCCESS;
  if (capacity < N) return XR_ERROR_SIZE_INSUFFICIENT;
  std::copy(values.begin(), values.end(), output);
  return XR_SUCCESS;
}

XrResult XRAPI_CALL EnumerateExtensions(const char *, uint32_t capacity,
                                        uint32_t *count,
                                        XrExtensionProperties *output) {
  std::array<XrExtensionProperties, 3> extensions{
      {{XR_TYPE_EXTENSION_PROPERTIES},
       {XR_TYPE_EXTENSION_PROPERTIES},
       {XR_TYPE_EXTENSION_PROPERTIES}}};
  strcpy_s(extensions[0].extensionName, XR_KHR_D3D11_ENABLE_EXTENSION_NAME);
  extensions[0].extensionVersion = XR_KHR_D3D11_enable_SPEC_VERSION;
  strcpy_s(extensions[1].extensionName, XR_KHR_D3D12_ENABLE_EXTENSION_NAME);
  extensions[1].extensionVersion = XR_KHR_D3D12_enable_SPEC_VERSION;
  strcpy_s(extensions[2].extensionName,
           XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME);
  extensions[2].extensionVersion = XR_KHR_composition_layer_depth_SPEC_VERSION;
  if (Config.DepthExtension)
    return Enumerate(extensions, capacity, count, output);
  return Enumerate(std::array{extensions[0], extensions[1]}, capacity, count,
                   output);
}
XrResult XRAPI_CALL CreateInstance(const XrInstanceCreateInfo *info,
                                   XrInstance *instance) {
  Stats.ApplicationInfo = info->applicationInfo;
  Stats.EnabledExtensions = info->enabledExtensionCount;
  Stats.DepthEnabled = false;
  for (UINT32 i = 0; i < info->enabledExtensionCount; ++i) {
    const auto name = info->enabledExtensionNames[i];
    if (strcmp(name, XR_KHR_COMPOSITION_LAYER_DEPTH_EXTENSION_NAME) == 0 &&
        Config.DepthExtension) {
      Stats.DepthEnabled = true;
    } else if (strcmp(name, XR_KHR_D3D11_ENABLE_EXTENSION_NAME) != 0 &&
               strcmp(name, XR_KHR_D3D12_ENABLE_EXTENSION_NAME) != 0) {
      return XR_ERROR_EXTENSION_NOT_PRESENT;
    }
    for (UINT32 j = 0; j < i; ++j)
      if (strcmp(name, info->enabledExtensionNames[j]) == 0) return Reject();
  }
  *instance = new XrInstance_T;
  ++Stats.InstancesCreated;
  ++Stats.LiveInstances;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL DestroyInstance(XrInstance instance) {
  if (Stats.LiveSessions) return Reject();
  RetiredInstances.emplace_back(instance);
  --Stats.LiveInstances;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL GetInstanceProperties(XrInstance,
                                          XrInstanceProperties *properties) {
  strcpy_s(properties->runtimeName, "MGDF integration runtime");
  properties->runtimeVersion = XR_MAKE_VERSION(1, 0, 0);
  return XR_SUCCESS;
}
XrResult XRAPI_CALL GetSystem(XrInstance, const XrSystemGetInfo *,
                              XrSystemId *system) {
  if (Config.NoHeadset) return XR_ERROR_FORM_FACTOR_UNAVAILABLE;
  *system = 1;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Requirements11(
    XrInstance, XrSystemId, XrGraphicsRequirementsD3D11KHR *requirements) {
  requirements->adapterLuid = Config.Adapter;
  requirements->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Requirements12(
    XrInstance, XrSystemId, XrGraphicsRequirementsD3D12KHR *requirements) {
  requirements->adapterLuid = Config.Adapter;
  requirements->minFeatureLevel = D3D_FEATURE_LEVEL_11_0;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL BlendModes(XrInstance, XrSystemId, XrViewConfigurationType,
                               uint32_t capacity, uint32_t *count,
                               XrEnvironmentBlendMode *modes) {
  return Enumerate(std::array{XR_ENVIRONMENT_BLEND_MODE_OPAQUE}, capacity,
                   count, modes);
}
XrResult XRAPI_CALL CreateSession(XrInstance, const XrSessionCreateInfo *info,
                                  XrSession *session) {
  if (Session) return Reject();
  const auto header = static_cast<const XrBaseInStructure *>(info->next);
  Session = new XrSession_T;
  if (header->type == XR_TYPE_GRAPHICS_BINDING_D3D11_KHR) {
    const auto binding =
        static_cast<const XrGraphicsBindingD3D11KHR *>(info->next);
    Session->Device11 = MakeComFromPtr<ID3D11Device>(binding->device);
  } else if (header->type == XR_TYPE_GRAPHICS_BINDING_D3D12_KHR) {
    const auto binding =
        static_cast<const XrGraphicsBindingD3D12KHR *>(info->next);
    Session->Device12 = MakeComFromPtr<ID3D12Device>(binding->device);
    Session->Queue = MakeComFromPtr<ID3D12CommandQueue>(binding->queue);
    if (binding->queue->GetDesc().Type != D3D12_COMMAND_LIST_TYPE_DIRECT)
      return Reject();
  } else
    return Reject();
  *session = Session;
  ++Stats.SessionsCreated;
  ++Stats.LiveSessions;
  Events.push_back({XR_SESSION_STATE_READY});
  Events.push_back({XR_SESSION_STATE_FOCUSED});
  return XR_SUCCESS;
}
XrResult XRAPI_CALL DestroySession(XrSession session) {
  if (Stats.LiveSwapchains || session->Begun) return Reject();
  *session = {};
  RetiredSessions.emplace_back(session);
  Session = XR_NULL_HANDLE;
  Events.clear();
  --Stats.LiveSessions;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL CreateSpace(XrSession session,
                                const XrReferenceSpaceCreateInfo *info,
                                XrSpace *space) {
  if (info->referenceSpaceType != XR_REFERENCE_SPACE_TYPE_LOCAL)
    return Reject();
  *space = new XrSpace_T{session};
  return XR_SUCCESS;
}
XrResult XRAPI_CALL DestroySpace(XrSpace space) {
  RetiredSpaces.emplace_back(space);
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Views(XrInstance, XrSystemId, XrViewConfigurationType type,
                          uint32_t capacity, uint32_t *count,
                          XrViewConfigurationView *views) {
  if (type != XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) return Reject();
  const std::array<XrViewConfigurationView, 2> values{
      {{XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr, 48, 128, 48, 128, 1, 1},
       {XR_TYPE_VIEW_CONFIGURATION_VIEW, nullptr, 64, 128, 48, 128, 1, 1}}};
  return Enumerate(values, capacity, count, views);
}
XrResult XRAPI_CALL Formats(XrSession, uint32_t capacity, uint32_t *count,
                            int64_t *formats) {
  return Enumerate(std::array<int64_t, 3>{DXGI_FORMAT_R8G8B8A8_UNORM,
                                          DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
                                          DXGI_FORMAT_D32_FLOAT},
                   capacity, count, formats);
}
XrResult XRAPI_CALL CreateSwapchain(XrSession session,
                                    const XrSwapchainCreateInfo *info,
                                    XrSwapchain *swapchain) {
  const bool depth = info->format == DXGI_FORMAT_D32_FLOAT;
  const auto usage = (depth ? XR_SWAPCHAIN_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
                            : XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT) |
                     XR_SWAPCHAIN_USAGE_SAMPLED_BIT |
                     XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
  if (info->arraySize != 1 || info->sampleCount != 1 ||
      (info->usageFlags & usage) != usage)
    return Reject();
  auto chain = std::make_unique<XrSwapchain_T>();
  chain->Info = *info;
  for (UINT32 i = 0; i < 3; ++i) {
    if (session->Device11) {
      const D3D11_TEXTURE2D_DESC desc{
          info->width,
          info->height,
          1,
          1,
          static_cast<DXGI_FORMAT>(info->format),
          {1, 0},
          D3D11_USAGE_DEFAULT,
          static_cast<UINT>(
              (depth ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET) |
              D3D11_BIND_SHADER_RESOURCE),
          0,
          0};
      ComObject<ID3D11Texture2D> image;
      if (FAILED(session->Device11->CreateTexture2D(&desc, nullptr,
                                                    image.Assign())))
        return XR_ERROR_RUNTIME_FAILURE;
      chain->Images11.push_back(image);
    } else {
      const D3D12_HEAP_PROPERTIES heap{D3D12_HEAP_TYPE_DEFAULT};
      const D3D12_RESOURCE_DESC desc{
          D3D12_RESOURCE_DIMENSION_TEXTURE2D,
          0,
          info->width,
          info->height,
          1,
          1,
          static_cast<DXGI_FORMAT>(info->format),
          {1, 0},
          D3D12_TEXTURE_LAYOUT_UNKNOWN,
          depth ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
                : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET};
      ComObject<ID3D12Resource> image;
      if (FAILED(session->Device12->CreateCommittedResource(
              &heap, D3D12_HEAP_FLAG_NONE, &desc,
              depth ? D3D12_RESOURCE_STATE_DEPTH_WRITE
                    : D3D12_RESOURCE_STATE_RENDER_TARGET,
              nullptr, IID_PPV_ARGS(image.Assign()))))
        return XR_ERROR_RUNTIME_FAILURE;
      chain->Images12.push_back(image);
    }
  }
  *swapchain = chain.release();
  ++Stats.LiveSwapchains;
  ++Stats.SwapchainsCreated;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL DestroySwapchain(XrSwapchain swapchain) {
  if (swapchain->Acquired) return Reject();
  delete swapchain;
  --Stats.LiveSwapchains;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Images(XrSwapchain swapchain, uint32_t capacity,
                           uint32_t *count,
                           XrSwapchainImageBaseHeader *images) {
  *count = 3;
  if (!capacity) return XR_SUCCESS;
  if (capacity < 3) return XR_ERROR_SIZE_INSUFFICIENT;
  for (UINT32 i = 0; i < 3; ++i) {
    if (!swapchain->Images11.empty())
      reinterpret_cast<XrSwapchainImageD3D11KHR *>(images)[i].texture =
          swapchain->Images11[i];
    else
      reinterpret_cast<XrSwapchainImageD3D12KHR *>(images)[i].texture =
          swapchain->Images12[i];
  }
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Poll(XrInstance, XrEventDataBuffer *buffer) {
  if (InstanceLost) {
    const XrEventDataInstanceLossPending event{
        XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING, nullptr, 1};
    memcpy(buffer, &event, sizeof(event));
    InstanceLost = false;
    return XR_SUCCESS;
  }
  if (Events.empty()) return XR_EVENT_UNAVAILABLE;
  const auto item = Events.front();
  Events.pop_front();
  if (item.Marker) {
    const XrEventDataEventsLost event{XR_TYPE_EVENT_DATA_EVENTS_LOST, nullptr,
                                      item.Marker};
    memcpy(buffer, &event, sizeof(event));
    // A synthetic extension chain exercises whole-buffer copies and relocation.
    auto chain = reinterpret_cast<XrBaseOutStructure *>(
        reinterpret_cast<char *>(buffer) + 128);
    *chain = {static_cast<XrStructureType>(1000999000), nullptr};
    buffer->next = chain;
    buffer->varying[sizeof(buffer->varying) - 1] =
        static_cast<uint8_t>(item.Marker);
    return XR_SUCCESS;
  }
  const auto state = item.State;
  Session->State = state;
  const XrEventDataSessionStateChanged event{
      XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED, nullptr, Session, state, 1};
  memcpy(buffer, &event, sizeof(event));
  return XR_SUCCESS;
}
XrResult XRAPI_CALL BeginSession(XrSession session,
                                 const XrSessionBeginInfo *info) {
  if (session->Running || session->State != XR_SESSION_STATE_READY ||
      info->primaryViewConfigurationType !=
          XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO)
    return Reject();
  session->Running = true;
  ++Stats.SessionsBegun;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL EndSession(XrSession session) {
  if (session->State != XR_SESSION_STATE_STOPPING || !session->Running ||
      session->Begun || session->Waited)
    return Reject();
  session->Running = false;
  ++Stats.SessionsEnded;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL RequestExit(XrSession session) {
  if (!session->Running) return Reject();
  ++Stats.ExitRequests;
  Events.push_back({XR_SESSION_STATE_STOPPING});
  return XR_SUCCESS;
}
XrResult XRAPI_CALL WaitFrame(XrSession session, const XrFrameWaitInfo *,
                              XrFrameState *state) {
  if (!session->Running || session->Waited || session->Begun) return Reject();
  session->Waited = true;
  state->predictedDisplayTime =
      (static_cast<XrTime>(Stats.Frames) + 1) * 11000000;
  state->predictedDisplayPeriod = 11000000;
  state->shouldRender = Config.ShouldRender;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL BeginFrame(XrSession session, const XrFrameBeginInfo *) {
  if (!session->Waited || session->Begun) return Reject();
  session->Begun = true;
  session->Waited = false;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Locate(XrSession session, const XrViewLocateInfo *info,
                           XrViewState *state, uint32_t capacity,
                           uint32_t *count, XrView *views) {
  if (!session->Begun || capacity != 2 || !info->space ||
      info->displayTime != (static_cast<XrTime>(Stats.Frames) + 1) * 11000000)
    return Reject();
  *count = 2;
  state->viewStateFlags =
      XR_VIEW_STATE_POSITION_VALID_BIT | XR_VIEW_STATE_ORIENTATION_VALID_BIT;
  for (UINT32 i = 0; i < 2; ++i) {
    views[i].pose = {{0, 0.2f, 0, 0.9797959f},
                     {i ? 0.03f : -0.03f, 1.6f, -1.0f}};
    views[i].fov = {-0.7f, 0.8f, 0.9f, -0.6f};
  }
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Acquire(XrSwapchain swapchain,
                            const XrSwapchainImageAcquireInfo *,
                            uint32_t *index) {
  if (!Session->Begun || swapchain->Acquired) return Reject();
  swapchain->Index = (swapchain->Index + 1) % 3;
  *index = swapchain->Index;
  swapchain->Acquired = true;
  swapchain->Released = false;
  if (Config.LossPendingOnAcquire) {
    Config.LossPendingOnAcquire = false;
    Events.push_back({XR_SESSION_STATE_LOSS_PENDING});
    return XR_SESSION_LOSS_PENDING;
  }
  return XR_SUCCESS;
}
XrResult XRAPI_CALL WaitImage(XrSwapchain swapchain,
                              const XrSwapchainImageWaitInfo *) {
  if (!swapchain->Acquired || swapchain->Waited) return Reject();
  if (Config.TimeoutFirstWait) {
    Config.TimeoutFirstWait = false;
    return XR_TIMEOUT_EXPIRED;
  }
  swapchain->Waited = true;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL Release(XrSwapchain swapchain,
                            const XrSwapchainImageReleaseInfo *) {
  if (!swapchain->Acquired || !swapchain->Waited) return Reject();
  swapchain->Acquired = swapchain->Waited = false;
  swapchain->Released = true;
  return XR_SUCCESS;
}
XrResult XRAPI_CALL EndFrame(XrSession session, const XrFrameEndInfo *info) {
  if (!session->Begun ||
      info->displayTime != (static_cast<XrTime>(Stats.Frames) + 1) * 11000000)
    return Reject();
  Stats.LastLayerCount = info->layerCount;
  std::fill(std::begin(Stats.LastLayers), std::end(Stats.LastLayers), 0);
  std::fill(std::begin(Stats.LastViewNext), std::end(Stats.LastViewNext), 0);
  if (info->layerCount) {
    if (info->layerCount > 4 ||
        info->layers[0]->type != XR_TYPE_COMPOSITION_LAYER_PROJECTION)
      return Reject();
    const auto layer =
        reinterpret_cast<const XrCompositionLayerProjection *>(info->layers[0]);
    if (layer->viewCount != 2 || layer->views[0].subImage.swapchain ==
                                     layer->views[1].subImage.swapchain)
      return Reject();
    for (UINT32 i = 0; i < 2; ++i) {
      const auto &view = layer->views[i];
      Stats.LastViewNext[i] = reinterpret_cast<UINT64>(view.next);
      if (view.next) {
        const auto depth =
            static_cast<const XrCompositionLayerDepthInfoKHR *>(view.next);
        if (!Stats.DepthEnabled ||
            depth->type != XR_TYPE_COMPOSITION_LAYER_DEPTH_INFO_KHR ||
            !depth->subImage.swapchain || !depth->subImage.swapchain->Released)
          return Reject();
        Stats.LastDepth[i] = *depth;
      }
      const auto chain = view.subImage.swapchain;
      if (!chain->Released || chain->Acquired ||
          view.pose.position.z != -1.0f ||
          view.subImage.imageRect.extent.width !=
              static_cast<int32_t>(chain->Info.width) ||
          view.subImage.imageArrayIndex != 0)
        return Reject();
    }
    for (UINT32 i = 1; i < info->layerCount; ++i) {
      if (info->layers[i]->type != XR_TYPE_COMPOSITION_LAYER_QUAD)
        return Reject();
      const auto quad =
          reinterpret_cast<const XrCompositionLayerQuad *>(info->layers[i]);
      if (!quad->subImage.swapchain || !quad->subImage.swapchain->Released)
        return Reject();
      Stats.LastLayers[i - 1] = reinterpret_cast<UINT64>(quad);
      Stats.LastQuads[i - 1] = *quad;
    }
    if (info->layerCount > 1) ++Stats.ExtraLayerFrames;
    ++Stats.ProjectionFrames;
    Stats.SubmittedImages += 2;
  } else
    ++Stats.EmptyFrames;
  ++Stats.Frames;
  session->Begun = false;
  return XR_SUCCESS;
}

XrResult XRAPI_CALL LocateSpace(XrSpace space, XrSpace base, XrTime time,
                                XrSpaceLocation *location) {
  if (!space || !base || space->Session != Session ||
      base->Session != Session ||
      time != (static_cast<XrTime>(Stats.Frames) + 1) * 11000000)
    return Reject();
  location->locationFlags = XR_SPACE_LOCATION_POSITION_VALID_BIT |
                            XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
  location->pose = {{0, 0, 0, 1}, {0, 0, 0}};
  return XR_SUCCESS;
}
XrResult XRAPI_CALL GetSystemProperties(XrInstance, XrSystemId system,
                                        XrSystemProperties *properties) {
  if (system != 1) return Reject();
  properties->systemId = system;
  strcpy_s(properties->systemName, "MGDF integration headset");
  properties->graphicsProperties = {128, 128, 4};
  return XR_SUCCESS;
}

XrResult XRAPI_CALL GetProc(XrInstance, const char *name,
                            PFN_xrVoidFunction *function) {
#define ENTRY(Name, Function)                                   \
  if (strcmp(name, Name) == 0) {                                \
    *function = reinterpret_cast<PFN_xrVoidFunction>(Function); \
    return XR_SUCCESS;                                          \
  }
  ENTRY("xrGetInstanceProcAddr", GetProc)
  ENTRY("xrEnumerateInstanceExtensionProperties", EnumerateExtensions)
  ENTRY("xrCreateInstance", CreateInstance)
  ENTRY("xrDestroyInstance", DestroyInstance)
  ENTRY("xrGetInstanceProperties", GetInstanceProperties)
  ENTRY("xrGetSystem", GetSystem)
  ENTRY("xrGetSystemProperties", GetSystemProperties)
  ENTRY("xrLocateSpace", LocateSpace)
  ENTRY("xrGetD3D11GraphicsRequirementsKHR", Requirements11)
  ENTRY("xrGetD3D12GraphicsRequirementsKHR", Requirements12)
  ENTRY("xrEnumerateEnvironmentBlendModes", BlendModes)
  ENTRY("xrCreateSession", CreateSession)
  ENTRY("xrDestroySession", DestroySession)
  ENTRY("xrCreateReferenceSpace", CreateSpace)
  ENTRY("xrDestroySpace", DestroySpace)
  ENTRY("xrEnumerateViewConfigurationViews", Views)
  ENTRY("xrEnumerateSwapchainFormats", Formats)
  ENTRY("xrCreateSwapchain", CreateSwapchain)
  ENTRY("xrDestroySwapchain", DestroySwapchain)
  ENTRY("xrEnumerateSwapchainImages", Images)
  ENTRY("xrPollEvent", Poll)
  ENTRY("xrBeginSession", BeginSession)
  ENTRY("xrEndSession", EndSession)
  ENTRY("xrRequestExitSession", RequestExit)
  ENTRY("xrWaitFrame", WaitFrame)
  ENTRY("xrBeginFrame", BeginFrame)
  ENTRY("xrLocateViews", Locate)
  ENTRY("xrAcquireSwapchainImage", Acquire)
  ENTRY("xrWaitSwapchainImage", WaitImage)
  ENTRY("xrReleaseSwapchainImage", Release)
  ENTRY("xrEndFrame", EndFrame)
#undef ENTRY
  *function = nullptr;
  return XR_ERROR_FUNCTION_UNSUPPORTED;
}
}  // namespace

// Loader negotiation ABI version 1, from Khronos openxr_loader_negotiation.h.
struct LoaderInfo {
  int Type;
  uint32_t Version;
  size_t Size;
  uint32_t MinInterfaceVersion, MaxInterfaceVersion;
  XrVersion MinApiVersion, MaxApiVersion;
};
struct RuntimeRequest {
  int Type;
  uint32_t Version;
  size_t Size;
  uint32_t InterfaceVersion;
  XrVersion ApiVersion;
  PFN_xrGetInstanceProcAddr GetProc;
};
extern "C" __declspec(dllexport) XrResult XRAPI_CALL
xrNegotiateLoaderRuntimeInterface(const LoaderInfo *loader,
                                  RuntimeRequest *runtime) {
  if (loader->MinInterfaceVersion > 1 || loader->MaxInterfaceVersion < 1)
    return XR_ERROR_INITIALIZATION_FAILED;
  runtime->InterfaceVersion = 1;
  runtime->ApiVersion = XR_MAKE_VERSION(1, 0, 0);
  runtime->GetProc = GetProc;
  return XR_SUCCESS;
}
extern "C" __declspec(dllexport) void Configure(const FakeXRConfig *config) {
  RetiredInstances.clear();
  RetiredSessions.clear();
  RetiredSpaces.clear();
  Config = *config;
  Stats = {};
  Events.clear();
  InstanceLost = false;
}
extern "C" __declspec(dllexport) void PushState(XrSessionState state) {
  Events.push_back({state});
}
extern "C" __declspec(dllexport) void SetShouldRender(bool render) {
  Config.ShouldRender = render;
}
extern "C" __declspec(dllexport) void GetStats(FakeXRStats *stats) {
  *stats = Stats;
}
extern "C" __declspec(dllexport) void LoseInstance() { InstanceLost = true; }

extern "C" __declspec(dllexport) void PushEvent(UINT32 marker) {
  Events.push_back({XR_SESSION_STATE_UNKNOWN, marker});
}
