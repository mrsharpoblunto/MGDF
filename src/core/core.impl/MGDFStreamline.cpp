#include "StdAfx.h"

#include "MGDFStreamline.hpp"

// The CRT debug heap defines free as a macro; Streamline declares a free
// method.
#pragma push_macro("free")
#undef free
#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_pcl.h>
#include <sl_reflex.h>
#pragma pop_macro("free")

#include <MGDF/D3D12CommandList.hpp>
#include <algorithm>
#include <optional>
#include <set>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFVersionInfo.hpp"

#if !defined(_DEBUG)
// Defines functions in the header, so only this file may include it.
#include <sl_security.h>
#pragma comment(lib, "crypt32.lib")
#endif

namespace MGDF {
namespace core {

namespace {

// Frame generation requires viewport 0.
const sl::ViewportHandle Viewport{0U};
constexpr char ProjectId[] = "3f1e8a52-6c0b-4d57-9e31-7b9d2a4c5f60";
constexpr UINT NvidiaVendorId = 0x10DE;

void StreamlineLog(sl::LogType type, const char *message) {
  if (!message) return;
  std::string text(message);
  while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
    text.pop_back();
  switch (type) {
    case sl::LogType::eError:
      LOG("Streamline: " << text, MGDF_LOG_ERROR);
      break;
    case sl::LogType::eWarn:
      LOG("Streamline: " << text, MGDF_LOG_MEDIUM);
      break;
    default:
      LOG("Streamline: " << text, MGDF_LOG_HIGH);
      break;
  }
}

constexpr sl::Boolean ToBoolean(BOOL value) {
  return value ? sl::Boolean::eTrue : sl::Boolean::eFalse;
}

sl::float4x4 ToMatrix(const FLOAT (&m)[16]) {
  sl::float4x4 result;
  for (uint32_t row = 0; row < 4; ++row) {
    result.setRow(row, sl::float4(m[row * 4], m[row * 4 + 1], m[row * 4 + 2],
                                  m[row * 4 + 3]));
  }
  return result;
}

std::optional<sl::DLSSMode> ToDLSSMode(MGDFUpscaleMode mode) {
  switch (mode) {
    case MGDF_UPSCALE_MODE_DLAA:
      return sl::DLSSMode::eDLAA;
    case MGDF_UPSCALE_MODE_QUALITY:
      return sl::DLSSMode::eMaxQuality;
    case MGDF_UPSCALE_MODE_BALANCED:
      return sl::DLSSMode::eBalanced;
    case MGDF_UPSCALE_MODE_PERFORMANCE:
      return sl::DLSSMode::eMaxPerformance;
    case MGDF_UPSCALE_MODE_ULTRA_PERFORMANCE:
      return sl::DLSSMode::eUltraPerformance;
    default:
      return std::nullopt;
  }
}

std::optional<sl::PCLMarker> ToPCLMarker(MGDFFrameMarker marker) {
  switch (marker) {
    case MGDF_FRAME_MARKER_SIMULATION_START:
      return sl::PCLMarker::eSimulationStart;
    case MGDF_FRAME_MARKER_SIMULATION_END:
      return sl::PCLMarker::eSimulationEnd;
    case MGDF_FRAME_MARKER_RENDER_SUBMIT_START:
      return sl::PCLMarker::eRenderSubmitStart;
    case MGDF_FRAME_MARKER_RENDER_SUBMIT_END:
      return sl::PCLMarker::eRenderSubmitEnd;
    // eInputSample is deprecated; this is the remaining input marker.
    case MGDF_FRAME_MARKER_INPUT_SAMPLE:
      return sl::PCLMarker::eControllerInputSample;
    default:
      return std::nullopt;
  }
}

sl::ReflexMode ToReflexMode(MGDFLowLatencyMode mode) {
  switch (mode) {
    case MGDF_LOW_LATENCY_ON:
      return sl::ReflexMode::eLowLatency;
    case MGDF_LOW_LATENCY_BOOST:
      return sl::ReflexMode::eLowLatencyWithBoost;
    default:
      return sl::ReflexMode::eOff;
  }
}

sl::Extent ToExtent(const MGDFFrameResource &resource, UINT32 defaultWidth,
                    UINT32 defaultHeight) {
  if (resource.Width && resource.Height) {
    return {resource.Top, resource.Left, resource.Width, resource.Height};
  }
  return {0, 0, defaultWidth, defaultHeight};
}

sl::Extent FullExtent(const MGDFFrameResource &resource) {
  const auto desc = resource.Resource->GetDesc();
  return ToExtent(resource, static_cast<UINT32>(desc.Width), desc.Height);
}

}  // namespace

struct StreamlineApi {
  PFun_slInit *Init = nullptr;
  PFun_slShutdown *Shutdown = nullptr;
  PFun_slSetD3DDevice *SetD3DDevice = nullptr;
  PFun_slIsFeatureSupported *IsFeatureSupported = nullptr;
  PFun_slSetFeatureLoaded *SetFeatureLoaded = nullptr;
  PFun_slGetFeatureFunction *GetFeatureFunction = nullptr;
  PFun_slUpgradeInterface *UpgradeInterface = nullptr;
  PFun_slGetNativeInterface *GetNativeInterface = nullptr;
  PFun_slGetNewFrameToken *GetNewFrameToken = nullptr;
  PFun_slSetConstants *SetConstants = nullptr;
  PFun_slSetTagForFrame *SetTagForFrame = nullptr;
  PFun_slEvaluateFeature *EvaluateFeature = nullptr;
};

// Everything except Api and Logged belongs to one slInit/slShutdown session.
struct StreamlineState {
  StreamlineApi Api;
  std::set<std::string> Logged;

  PFun_slDLSSGetOptimalSettings *DLSSGetOptimalSettings = nullptr;
  PFun_slDLSSSetOptions *DLSSSetOptions = nullptr;
  PFun_slDLSSGSetOptions *DLSSGSetOptions = nullptr;
  PFun_slDLSSGGetState *DLSSGGetState = nullptr;
  PFun_slReflexSetOptions *ReflexSetOptions = nullptr;
  PFun_slReflexSleep *ReflexSleep = nullptr;
  PFun_slReflexGetState *ReflexGetState = nullptr;
  PFun_slPCLSetMarker *PCLSetMarker = nullptr;
  PFun_slPCLGetState *PCLGetState = nullptr;

  bool Active = false;
  UINT32 Supported = 0;
  UINT32 MaxGeneratedFrames = 0;
  bool FrameGenerationVSync = false;
  bool Reflex = false;
  bool PCL = false;
  ComObject<ID3D12Device> ProxyDevice;

  sl::FrameToken *Token = nullptr;
  bool ConstantsSet = false;
  std::optional<sl::ReflexMode> ReflexMode;
  std::optional<sl::DLSSOptions> DLSSOptions;

  bool FrameGeneration = false;
  bool FrameGenerationFailed = false;
  bool FrameGenerationSuspended = false;
  bool FrameGenerationInputs = false;
  bool FrameGenerationTagged = false;
  UINT32 GeneratedFrames = 0;
  std::optional<sl::DLSSGMode> FrameGenerationMode;

  UINT64 PresentedFrames = 0;
  UINT64 RenderedFrames = 0;
  bool HasLatency = false;
  double RenderLatency = 0.0;

  template <typename T>
  bool Import(sl::Feature feature, const char *name, T *&function) {
    void *pointer = nullptr;
    function = nullptr;
    if (Api.GetFeatureFunction(feature, name, pointer) != sl::Result::eOk ||
        !pointer)
      return false;
    function = reinterpret_cast<T *>(pointer);
    return true;
  }
};

Streamline::Streamline(UINT32 requestedFeatures, std::wstring pluginDirectory)
    : _requested(requestedFeatures),
      _directory(std::move(pluginDirectory)),
      _state(std::make_unique<StreamlineState>()) {}

Streamline::~Streamline() {
  {
    std::lock_guard lock(_mutex);
    ShutdownLocked();
  }
  if (_interposer) FreeLibrary(_interposer);
}

void Streamline::LogOnce(const std::string &key, const std::string &message,
                         MGDFLogLevel level) const {
  if (_state->Logged.insert(key).second) LOG(message, level);
}

bool Streamline::LoadInterposer() {
  if (_interposer) return true;
  const std::wstring path = _directory + L"sl.interposer.dll";
  if (GetFileAttributesW(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
    LogOnce("missing",
            "Streamline is not available: sl.interposer.dll is missing",
            MGDF_LOG_LOW);
    return false;
  }
#if !defined(_DEBUG)
  if (!sl::security::verifyEmbeddedSignature(path.c_str())) {
    LogOnce("signature",
            "Streamline is not available: sl.interposer.dll has no valid "
            "NVIDIA signature",
            MGDF_LOG_ERROR);
    return false;
  }
#endif
  _interposer = LoadLibraryW(path.c_str());
  if (!_interposer) {
    LogOnce("load",
            "Streamline is not available: cannot load sl.interposer.dll (" +
                std::to_string(GetLastError()) + ")",
            MGDF_LOG_ERROR);
    return false;
  }
  auto &s = _state->Api;
  const auto get = [this](auto &function, const char *name) {
    function = reinterpret_cast<std::remove_reference_t<decltype(function)>>(
        GetProcAddress(_interposer, name));
    return function != nullptr;
  };
  if (!get(s.Init, "slInit") || !get(s.Shutdown, "slShutdown") ||
      !get(s.SetD3DDevice, "slSetD3DDevice") ||
      !get(s.IsFeatureSupported, "slIsFeatureSupported") ||
      !get(s.SetFeatureLoaded, "slSetFeatureLoaded") ||
      !get(s.GetFeatureFunction, "slGetFeatureFunction") ||
      !get(s.UpgradeInterface, "slUpgradeInterface") ||
      !get(s.GetNativeInterface, "slGetNativeInterface") ||
      !get(s.GetNewFrameToken, "slGetNewFrameToken") ||
      !get(s.SetConstants, "slSetConstants") ||
      !get(s.SetTagForFrame, "slSetTagForFrame") ||
      !get(s.EvaluateFeature, "slEvaluateFeature")) {
    LogOnce(
        "exports",
        "Streamline is not available: sl.interposer.dll exports are missing",
        MGDF_LOG_ERROR);
    FreeLibrary(_interposer);
    _interposer = nullptr;
    return false;
  }
  return true;
}

void Streamline::RTInit(ID3D12Device *device, IDXGIAdapter1 *adapter) {
  std::lock_guard lock(_mutex);
  ShutdownLocked();
  if (!_requested || !device || !adapter) return;

  DXGI_ADAPTER_DESC1 adapterDesc{};
  if (FAILED(adapter->GetDesc1(&adapterDesc))) return;
  if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
    LogOnce("software", "Streamline is not available on a software adapter",
            MGDF_LOG_LOW);
    return;
  }
  if (adapterDesc.VendorId != NvidiaVendorId) {
    LogOnce("vendor", "Streamline is not available on a non-NVIDIA adapter",
            MGDF_LOG_LOW);
    return;
  }
  if (!LoadInterposer()) return;
  auto &s = *_state;

  std::vector<sl::Feature> features;
  const bool frameGeneration =
      (_requested & MGDF_GRAPHICS_FEATURE_FRAME_GENERATION) != 0;
  if (_requested & MGDF_GRAPHICS_FEATURE_UPSCALE)
    features.push_back(sl::kFeatureDLSS);
  if (frameGeneration) features.push_back(sl::kFeatureDLSS_G);
  if (frameGeneration || (_requested & MGDF_GRAPHICS_FEATURE_LOW_LATENCY)) {
    features.push_back(sl::kFeatureReflex);
    features.push_back(sl::kFeaturePCL);
  }

  const wchar_t *paths[] = {_directory.c_str()};
  const std::string version = MGDFVersionInfo::MGDF_VERSION();
  sl::Preferences preferences{};
  preferences.showConsole = false;
  preferences.logLevel = sl::LogLevel::eDefault;
  preferences.pathsToPlugins = paths;
  preferences.numPathsToPlugins = 1;
  preferences.pathToLogsAndData = nullptr;
  preferences.logMessageCallback = StreamlineLog;
  // No OTA: the shipped plugins and NGX models are the ones that run.
  preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking |
                      sl::PreferenceFlags::eDisableDebugText |
                      sl::PreferenceFlags::eUseManualHooking |
                      sl::PreferenceFlags::eUseDXGIFactoryProxy |
                      sl::PreferenceFlags::eUseFrameBasedResourceTagging;
  preferences.featuresToLoad = features.data();
  preferences.numFeaturesToLoad = static_cast<uint32_t>(features.size());
  preferences.engine = sl::EngineType::eCustom;
  preferences.engineVersion = version.c_str();
  preferences.projectId = ProjectId;
  preferences.renderAPI = sl::RenderAPI::eD3D12;

  auto result = s.Api.Init(preferences, sl::kSDKVersion);
  if (result != sl::Result::eOk) {
    LogOnce("init",
            "Streamline is not available: slInit failed (" +
                std::to_string(static_cast<int>(result)) + ")",
            MGDF_LOG_ERROR);
    s.Api.Shutdown();
    return;
  }
  s.Active = true;
  result = s.Api.SetD3DDevice(device);
  if (result != sl::Result::eOk) {
    LogOnce("device",
            "Streamline is not available: slSetD3DDevice failed (" +
                std::to_string(static_cast<int>(result)) + ")",
            MGDF_LOG_ERROR);
    ShutdownLocked();
    return;
  }

  sl::AdapterInfo adapterInfo{};
  adapterInfo.deviceLUID =
      reinterpret_cast<uint8_t *>(&adapterDesc.AdapterLuid);
  adapterInfo.deviceLUIDSizeInBytes = sizeof(LUID);
  const auto supported = [&](sl::Feature feature, const char *name) {
    const auto support = s.Api.IsFeatureSupported(feature, adapterInfo);
    if (support != sl::Result::eOk) {
      LogOnce(std::string("unsupported-") + name,
              std::string("Streamline ") + name + " is not supported (" +
                  std::to_string(static_cast<int>(support)) + ")",
              MGDF_LOG_LOW);
      return false;
    }
    return true;
  };

  if ((_requested & MGDF_GRAPHICS_FEATURE_UPSCALE) &&
      supported(sl::kFeatureDLSS, "DLSS") &&
      s.Import(sl::kFeatureDLSS, "slDLSSGetOptimalSettings",
               s.DLSSGetOptimalSettings) &&
      s.Import(sl::kFeatureDLSS, "slDLSSSetOptions", s.DLSSSetOptions)) {
    s.Supported |= MGDF_GRAPHICS_FEATURE_UPSCALE;
  }
  s.PCL = std::ranges::find(features, sl::kFeaturePCL) != features.end() &&
          supported(sl::kFeaturePCL, "PCL") &&
          s.Import(sl::kFeaturePCL, "slPCLSetMarker", s.PCLSetMarker) &&
          s.Import(sl::kFeaturePCL, "slPCLGetState", s.PCLGetState);
  s.Reflex =
      s.PCL && supported(sl::kFeatureReflex, "Reflex") &&
      s.Import(sl::kFeatureReflex, "slReflexSetOptions", s.ReflexSetOptions) &&
      s.Import(sl::kFeatureReflex, "slReflexSleep", s.ReflexSleep) &&
      s.Import(sl::kFeatureReflex, "slReflexGetState", s.ReflexGetState);
  if (s.Reflex) {
    sl::ReflexState reflex{};
    if ((_requested & MGDF_GRAPHICS_FEATURE_LOW_LATENCY) &&
        s.ReflexGetState(reflex) == sl::Result::eOk &&
        reflex.lowLatencyAvailable) {
      s.Supported |= MGDF_GRAPHICS_FEATURE_LOW_LATENCY;
    }
    // Reflex must have options once, even when it stays off.
    sl::ReflexOptions options{};
    options.mode = sl::ReflexMode::eOff;
    if (s.ReflexSetOptions(options) == sl::Result::eOk)
      s.ReflexMode = sl::ReflexMode::eOff;
  }
  if (frameGeneration && s.Reflex && supported(sl::kFeatureDLSS_G, "DLSS-G") &&
      s.Import(sl::kFeatureDLSS_G, "slDLSSGSetOptions", s.DLSSGSetOptions) &&
      s.Import(sl::kFeatureDLSS_G, "slDLSSGGetState", s.DLSSGGetState)) {
    sl::DLSSGState state{};
    if (s.DLSSGGetState(Viewport, state, nullptr) == sl::Result::eOk &&
        state.numFramesToGenerateMax > 0) {
      s.Supported |= MGDF_GRAPHICS_FEATURE_FRAME_GENERATION;
      s.MaxGeneratedFrames = state.numFramesToGenerateMax;
      s.FrameGenerationVSync =
          state.bIsVsyncSupportAvailable == sl::Boolean::eTrue;
    }
  }

  if (!s.Supported) {
    LogOnce("none", "Streamline supports none of the requested features",
            MGDF_LOG_LOW);
    ShutdownLocked();
    return;
  }

  void *proxy = device;
  device->AddRef();
  if (s.Api.UpgradeInterface(&proxy) != sl::Result::eOk) {
    device->Release();
    LogOnce("proxy", "Streamline is not available: device upgrade failed",
            MGDF_LOG_ERROR);
    ShutdownLocked();
    return;
  }
  // The proxy keeps its own reference to the device; ours goes with it.
  if (proxy != device) device->Release();
  s.ProxyDevice = ComObject<ID3D12Device>(static_cast<ID3D12Device *>(proxy));

  if (s.PCL) {
    sl::PCLState pcl{};
    if (s.PCLGetState(pcl) == sl::Result::eOk)
      _pingMessage.store(pcl.statsWindowMessage);
  }
  LOG("Streamline active: upscale "
          << ((s.Supported & MGDF_GRAPHICS_FEATURE_UPSCALE) ? "yes" : "no")
          << ", low latency "
          << ((s.Supported & MGDF_GRAPHICS_FEATURE_LOW_LATENCY) ? "yes" : "no")
          << ", generated frames " << s.MaxGeneratedFrames,
      MGDF_LOG_LOW);
}

void Streamline::ShutdownLocked() {
  auto &s = *_state;
  _pingMessage.store(0);
  _ping.store(false);
  if (s.Active) {
    s.ProxyDevice.Clear();
    s.Api.Shutdown();
  }
  StreamlineState reset;
  reset.Api = s.Api;
  reset.Logged = std::move(s.Logged);
  s = std::move(reset);
}

void Streamline::RTShutdown() {
  std::lock_guard lock(_mutex);
  ShutdownLocked();
}

bool Streamline::RTIsActive() const {
  std::lock_guard lock(_mutex);
  return _state->Active;
}

ComObject<ID3D12Device> Streamline::RTGetProxyDevice() const {
  std::lock_guard lock(_mutex);
  return _state->ProxyDevice;
}

ComObject<IDXGIFactory2> Streamline::RTUpgradeFactory(IDXGIFactory2 *factory) {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!s.Active || !factory) return {};
  void *proxy = factory;
  factory->AddRef();
  if (s.Api.UpgradeInterface(&proxy) != sl::Result::eOk) {
    factory->Release();
    return {};
  }
  if (proxy != factory) factory->Release();
  ComObject<IUnknown> unknown(static_cast<IUnknown *>(proxy));
  return unknown.As<IDXGIFactory2>();
}

ComObject<IUnknown> Streamline::RTGetNativeUnknown(IUnknown *proxy) {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!s.Active || !proxy) return {};
  void *native = nullptr;
  if (s.Api.GetNativeInterface(proxy, &native) != sl::Result::eOk || !native)
    return {};
  return ComObject<IUnknown>(static_cast<IUnknown *>(native));
}

MGDFGraphicsFeatureSupport Streamline::GetSupport() const {
  std::lock_guard lock(_mutex);
  const auto &s = *_state;
  return {.Features = s.Supported,
          .MaxGeneratedFrames = s.MaxGeneratedFrames,
          .FrameGenerationVSync = s.FrameGenerationVSync ? TRUE : FALSE};
}

HRESULT Streamline::GetUpscaleRenderSize(MGDFUpscaleMode mode,
                                         UINT32 displayWidth,
                                         UINT32 displayHeight,
                                         MGDFUpscaleRenderSize *size) const {
  std::lock_guard lock(_mutex);
  const auto &s = *_state;
  if (!(s.Supported & MGDF_GRAPHICS_FEATURE_UPSCALE)) return E_NOTIMPL;
  const auto dlssMode = ToDLSSMode(mode);
  if (!size || !dlssMode || !displayWidth || !displayHeight)
    return E_INVALIDARG;
  sl::DLSSOptions options{};
  options.mode = *dlssMode;
  options.outputWidth = displayWidth;
  options.outputHeight = displayHeight;
  sl::DLSSOptimalSettings settings{};
  if (s.DLSSGetOptimalSettings(options, settings) != sl::Result::eOk ||
      !settings.optimalRenderWidth || !settings.optimalRenderHeight)
    return E_FAIL;
  *size = {.Width = settings.optimalRenderWidth,
           .Height = settings.optimalRenderHeight,
           .MinWidth = settings.renderWidthMin ? settings.renderWidthMin
                                               : settings.optimalRenderWidth,
           .MinHeight = settings.renderHeightMin ? settings.renderHeightMin
                                                 : settings.optimalRenderHeight,
           .MaxWidth = settings.renderWidthMax ? settings.renderWidthMax
                                               : settings.optimalRenderWidth,
           .MaxHeight = settings.renderHeightMax
                            ? settings.renderHeightMax
                            : settings.optimalRenderHeight};
  return S_OK;
}

UINT Streamline::GetLatencyPingMessage() const { return _pingMessage.load(); }

void Streamline::QueueLatencyPing() { _ping.store(true); }

StreamlineFrameStats Streamline::GetFrameStats() const {
  std::lock_guard lock(_mutex);
  const auto &s = *_state;
  return {.HasLatency = s.HasLatency,
          .RenderLatency = s.RenderLatency,
          .PresentedFrames = s.PresentedFrames};
}

bool Streamline::RTPrepareSwapChain(UINT32 generatedFrames,
                                    DXGI_FORMAT format) {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  s.FrameGeneration = false;
  s.FrameGenerationFailed = false;
  s.FrameGenerationSuspended = false;
  s.FrameGenerationInputs = false;
  s.FrameGenerationTagged = false;
  // The old swap chain was released with frame generation suspended, so the
  // plugin keeps eOff; a second eOff call in this frame only adds a warning.
  if (s.FrameGenerationMode != sl::DLSSGMode::eOff)
    s.FrameGenerationMode.reset();
  if (!s.Active || !(s.Supported & MGDF_GRAPHICS_FEATURE_FRAME_GENERATION))
    return false;
  bool enable = generatedFrames > 0;
  // DLSS-G supports SDR and HDR10 back buffers, but not scRGB.
  if (enable && format == DXGI_FORMAT_R16G16B16A16_FLOAT) {
    LogOnce("fp16", "Frame generation does not support scRGB HDR output",
            MGDF_LOG_LOW);
    enable = false;
  }
  // Without DLSS-G loaded the swap chain has no frame generation overhead.
  if (s.Api.SetFeatureLoaded(sl::kFeatureDLSS_G, enable) != sl::Result::eOk) {
    LogOnce("loaded", "Cannot change the DLSS-G plugin state", MGDF_LOG_ERROR);
    return false;
  }
  if (enable &&
      (!s.Import(sl::kFeatureDLSS_G, "slDLSSGSetOptions", s.DLSSGSetOptions) ||
       !s.Import(sl::kFeatureDLSS_G, "slDLSSGGetState", s.DLSSGGetState))) {
    s.Api.SetFeatureLoaded(sl::kFeatureDLSS_G, false);
    return false;
  }
  s.FrameGeneration = enable;
  s.GeneratedFrames = (std::min)(generatedFrames, s.MaxGeneratedFrames);
  return enable;
}

void Streamline::ClearFrameGenerationTagsLocked() {
  auto &s = *_state;
  if (!s.FrameGenerationTagged || !s.Token) return;
  const sl::ResourceTag tags[] = {{nullptr, sl::kBufferTypeDepth,
                                   sl::ResourceLifecycle::eValidUntilPresent},
                                  {nullptr, sl::kBufferTypeMotionVectors,
                                   sl::ResourceLifecycle::eValidUntilPresent},
                                  {nullptr, sl::kBufferTypeHUDLessColor,
                                   sl::ResourceLifecycle::eValidUntilPresent},
                                  {nullptr, sl::kBufferTypeUIColorAndAlpha,
                                   sl::ResourceLifecycle::eValidUntilPresent}};
  s.Api.SetTagForFrame(*s.Token, Viewport, tags, _countof(tags), nullptr);
  s.FrameGenerationTagged = false;
}

void Streamline::RTSuspendFrameGeneration() {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!s.FrameGeneration) return;
  s.FrameGenerationSuspended = true;
  ClearFrameGenerationTagsLocked();
  if (s.FrameGenerationMode != sl::DLSSGMode::eOff) {
    sl::DLSSGOptions options{};
    options.mode = sl::DLSSGMode::eOff;
    options.flags = sl::DLSSGFlags::eRetainResourcesWhenOff;
    if (s.DLSSGSetOptions(Viewport, options) == sl::Result::eOk)
      s.FrameGenerationMode = sl::DLSSGMode::eOff;
  }
}

void Streamline::RTResumeFrameGeneration() {
  std::lock_guard lock(_mutex);
  _state->FrameGenerationSuspended = false;
}

UINT Streamline::RTGetSyncInterval(UINT requested) const {
  std::lock_guard lock(_mutex);
  const auto &s = *_state;
  if (s.FrameGeneration && !s.FrameGenerationVSync) return 0;
  return (std::min)(requested, 1U);
}

void Streamline::RTBeginFrame(MGDFLowLatencyMode lowLatency) {
  sl::FrameToken *token = nullptr;
  bool sleep = false;
  {
    std::lock_guard lock(_mutex);
    auto &s = *_state;
    if (!s.Active) return;
    s.ConstantsSet = false;
    s.FrameGenerationInputs = false;
    if (s.Api.GetNewFrameToken(s.Token, nullptr) != sl::Result::eOk) {
      s.Token = nullptr;
      LogOnce("token", "Streamline cannot create a frame token",
              MGDF_LOG_ERROR);
      return;
    }
    token = s.Token;
    if (s.Reflex) {
      auto mode = (s.Supported & MGDF_GRAPHICS_FEATURE_LOW_LATENCY)
                      ? ToReflexMode(lowLatency)
                      : sl::ReflexMode::eOff;
      // Frame generation fails without Reflex low latency.
      if (s.FrameGeneration && mode == sl::ReflexMode::eOff)
        mode = sl::ReflexMode::eLowLatency;
      if (s.ReflexMode != mode) {
        sl::ReflexOptions options{};
        options.mode = mode;
        if (s.ReflexSetOptions(options) == sl::Result::eOk) s.ReflexMode = mode;
      }
      sleep = true;
    }
    if (s.PCL && _ping.exchange(false))
      s.PCLSetMarker(sl::PCLMarker::ePCLatencyPing, *token);
  }
  // Reflex expects the sleep in every frame, also when its mode is off.
  if (sleep) _state->ReflexSleep(*token);
}

void Streamline::RTSetFrameMarker(MGDFFrameMarker marker) {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  const auto pcl = ToPCLMarker(marker);
  if (!s.PCL || !s.Token || !pcl) return;
  s.PCLSetMarker(*pcl, *s.Token);
}

void Streamline::RTSetFrameConstants(const MGDFFrameConstants *constants) {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!constants || !s.Token ||
      !(s.Supported & (MGDF_GRAPHICS_FEATURE_UPSCALE |
                       MGDF_GRAPHICS_FEATURE_FRAME_GENERATION)))
    return;
  if (s.ConstantsSet) {
    LogOnce("constants",
            "SetFrameConstants was called more than once in a frame",
            MGDF_LOG_ERROR);
    return;
  }
  sl::Constants c{};
  c.cameraViewToClip = ToMatrix(constants->ViewToClip);
  c.clipToCameraView = ToMatrix(constants->ClipToView);
  c.clipToLensClip = ToMatrix({1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1});
  c.clipToPrevClip = ToMatrix(constants->ClipToPreviousClip);
  c.prevClipToClip = ToMatrix(constants->PreviousClipToClip);
  c.jitterOffset = {constants->JitterOffset[0], constants->JitterOffset[1]};
  c.mvecScale = {constants->MotionVectorScale[0],
                 constants->MotionVectorScale[1]};
  c.cameraPinholeOffset = {0.0f, 0.0f};
  c.cameraPos = {constants->CameraPosition[0], constants->CameraPosition[1],
                 constants->CameraPosition[2]};
  c.cameraUp = {constants->CameraUp[0], constants->CameraUp[1],
                constants->CameraUp[2]};
  c.cameraRight = {constants->CameraRight[0], constants->CameraRight[1],
                   constants->CameraRight[2]};
  c.cameraFwd = {constants->CameraForward[0], constants->CameraForward[1],
                 constants->CameraForward[2]};
  c.cameraNear = constants->Near;
  c.cameraFar = constants->Far;
  c.cameraFOV = constants->VerticalFOV;
  c.cameraAspectRatio = constants->AspectRatio;
  c.depthInverted = ToBoolean(constants->DepthInverted);
  c.cameraMotionIncluded = ToBoolean(constants->CameraMotionIncluded);
  c.motionVectors3D = sl::Boolean::eFalse;
  c.reset = ToBoolean(constants->ResetHistory);
  c.orthographicProjection = sl::Boolean::eFalse;
  c.motionVectorsDilated = ToBoolean(constants->MotionVectorsDilated);
  c.motionVectorsJittered = ToBoolean(constants->MotionVectorsJittered);
  if (s.Api.SetConstants(c, *s.Token, Viewport) == sl::Result::eOk) {
    s.ConstantsSet = true;
  } else {
    LogOnce("constants-failed", "Streamline rejected the frame constants",
            MGDF_LOG_ERROR);
  }
}

HRESULT Streamline::RTUpscale(ID3D12GraphicsCommandList *list,
                              const MGDFUpscaleDesc *desc) {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!(s.Supported & MGDF_GRAPHICS_FEATURE_UPSCALE) || !s.Token)
    return E_NOTIMPL;
  if (!list || !desc || list->GetType() != D3D12_COMMAND_LIST_TYPE_DIRECT ||
      !D3D12CommandListIsRecording(list) || !desc->ColorIn.Resource ||
      !desc->ColorOut.Resource || !desc->Depth.Resource ||
      !desc->MotionVectors.Resource || !desc->RenderWidth ||
      !desc->RenderHeight || !desc->OutputWidth || !desc->OutputHeight)
    return E_INVALIDARG;
  const auto mode = ToDLSSMode(desc->Mode);
  if (!mode) return E_INVALIDARG;
  if (!s.ConstantsSet) return E_ILLEGAL_METHOD_CALL;

  sl::DLSSOptions options{};
  options.mode = *mode;
  options.outputWidth = desc->OutputWidth;
  options.outputHeight = desc->OutputHeight;
  options.colorBuffersHDR = ToBoolean(desc->ColorHDR);
  options.useAutoExposure = ToBoolean(!desc->Exposure.Resource);
  if (!s.DLSSOptions || s.DLSSOptions->mode != options.mode ||
      s.DLSSOptions->outputWidth != options.outputWidth ||
      s.DLSSOptions->outputHeight != options.outputHeight ||
      s.DLSSOptions->colorBuffersHDR != options.colorBuffersHDR ||
      s.DLSSOptions->useAutoExposure != options.useAutoExposure) {
    if (s.DLSSSetOptions(Viewport, options) != sl::Result::eOk) {
      LogOnce("dlss-options", "Streamline rejected the DLSS options",
              MGDF_LOG_ERROR);
      return E_FAIL;
    }
    s.DLSSOptions = options;
  }

  struct Input {
    const MGDFFrameResource &Resource;
    sl::BufferType Type;
    UINT32 Width;
    UINT32 Height;
  };
  const Input inputs[] = {
      {desc->ColorIn, sl::kBufferTypeScalingInputColor, desc->RenderWidth,
       desc->RenderHeight},
      {desc->ColorOut, sl::kBufferTypeScalingOutputColor, desc->OutputWidth,
       desc->OutputHeight},
      {desc->Depth, sl::kBufferTypeDepth, desc->RenderWidth,
       desc->RenderHeight},
      {desc->MotionVectors, sl::kBufferTypeMotionVectors, desc->RenderWidth,
       desc->RenderHeight},
      {desc->Exposure, sl::kBufferTypeExposure, 1, 1},
      {desc->BiasCurrentColor, sl::kBufferTypeBiasCurrentColorHint,
       desc->RenderWidth, desc->RenderHeight}};
  std::vector<sl::Resource> resources;
  std::vector<sl::ResourceTag> tags;
  resources.reserve(_countof(inputs));
  tags.reserve(_countof(inputs));
  for (const auto &input : inputs) {
    if (!input.Resource.Resource) continue;
    resources.emplace_back(sl::ResourceType::eTex2d, input.Resource.Resource,
                           static_cast<uint32_t>(input.Resource.State));
    const auto extent = ToExtent(input.Resource, input.Width, input.Height);
    tags.emplace_back(&resources.back(), input.Type,
                      sl::ResourceLifecycle::eValidUntilEvaluate, &extent);
  }
  std::vector<const sl::BaseStructure *> evaluateInputs{&Viewport};
  for (const auto &tag : tags) evaluateInputs.push_back(&tag);
  const auto result =
      s.Api.EvaluateFeature(sl::kFeatureDLSS, *s.Token, evaluateInputs.data(),
                            static_cast<uint32_t>(evaluateInputs.size()), list);
  if (result != sl::Result::eOk) {
    LogOnce("evaluate",
            "DLSS evaluation failed (" +
                std::to_string(static_cast<int>(result)) + ")",
            MGDF_LOG_ERROR);
    return E_FAIL;
  }
  return S_OK;
}

HRESULT Streamline::RTSetFrameGenerationInputs(
    const MGDFFrameGenerationDesc *desc) {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!s.FrameGeneration || !s.Token) return S_FALSE;
  if (desc && (!desc->Depth.Resource || !desc->MotionVectors.Resource))
    return E_INVALIDARG;
  if (!desc) {
    s.FrameGenerationInputs = false;
    return S_OK;
  }
  const MGDFFrameResource *inputs[] = {&desc->Depth, &desc->MotionVectors,
                                       &desc->HudlessColor,
                                       &desc->UIColorAndAlpha};
  const sl::BufferType types[] = {
      sl::kBufferTypeDepth, sl::kBufferTypeMotionVectors,
      sl::kBufferTypeHUDLessColor, sl::kBufferTypeUIColorAndAlpha};
  sl::Resource resources[_countof(inputs)] = {
      {sl::ResourceType::eTex2d, nullptr},
      {sl::ResourceType::eTex2d, nullptr},
      {sl::ResourceType::eTex2d, nullptr},
      {sl::ResourceType::eTex2d, nullptr}};
  std::vector<sl::ResourceTag> tags;
  for (size_t i = 0; i < _countof(inputs); ++i) {
    // A null tag releases the reference Streamline keeps from earlier frames.
    if (!inputs[i]->Resource) {
      tags.emplace_back(nullptr, types[i],
                        sl::ResourceLifecycle::eValidUntilPresent);
      continue;
    }
    resources[i] = {sl::ResourceType::eTex2d, inputs[i]->Resource,
                    static_cast<uint32_t>(inputs[i]->State)};
    const auto extent = FullExtent(*inputs[i]);
    tags.emplace_back(&resources[i], types[i],
                      sl::ResourceLifecycle::eValidUntilPresent, &extent);
  }
  if (s.Api.SetTagForFrame(*s.Token, Viewport, tags.data(),
                           static_cast<uint32_t>(tags.size()),
                           nullptr) != sl::Result::eOk) {
    LogOnce("tags", "Streamline rejected the frame generation inputs",
            MGDF_LOG_ERROR);
    return E_FAIL;
  }
  s.FrameGenerationTagged = true;
  s.FrameGenerationInputs = true;
  return S_OK;
}

void Streamline::RTBeforePresent() {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!s.Active || !s.Token) return;
  if (s.FrameGeneration) {
    const bool run = s.FrameGenerationInputs && s.ConstantsSet &&
                     !s.FrameGenerationSuspended && !s.FrameGenerationFailed;
    if (!run) ClearFrameGenerationTagsLocked();
    const auto mode = run ? sl::DLSSGMode::eOn : sl::DLSSGMode::eOff;
    if (s.FrameGenerationMode != mode) {
      sl::DLSSGOptions options{};
      options.mode = mode;
      options.numFramesToGenerate = (std::max)(s.GeneratedFrames, 1U);
      options.flags = sl::DLSSGFlags::eRetainResourcesWhenOff;
      const auto result = s.DLSSGSetOptions(Viewport, options);
      if (result == sl::Result::eOk) {
        s.FrameGenerationMode = mode;
        LOG("Frame generation " << (run ? "running" : "paused"), MGDF_LOG_HIGH);
      } else {
        LogOnce("dlssg-options",
                "Streamline rejected the frame generation options (" +
                    std::to_string(static_cast<int>(result)) + ")",
                MGDF_LOG_ERROR);
      }
    }
  }
  if (s.PCL) s.PCLSetMarker(sl::PCLMarker::ePresentStart, *s.Token);
}

void Streamline::RTAfterPresent() {
  std::lock_guard lock(_mutex);
  auto &s = *_state;
  if (!s.Active || !s.Token) return;
  if (s.PCL) s.PCLSetMarker(sl::PCLMarker::ePresentEnd, *s.Token);
  ++s.RenderedFrames;
  if (s.FrameGeneration) {
    sl::DLSSGState state{};
    if (s.DLSSGGetState(Viewport, state, nullptr) == sl::Result::eOk) {
      s.PresentedFrames += state.numFramesActuallyPresented;
      if (state.status != sl::DLSSGStatus::eOk &&
          s.FrameGenerationMode == sl::DLSSGMode::eOn) {
        LOG("Frame generation stopped until the next swap chain (status "
                << static_cast<uint32_t>(state.status) << ")",
            MGDF_LOG_ERROR);
        s.FrameGenerationFailed = true;
      }
    }
  } else {
    ++s.PresentedFrames;
  }
  if (s.Reflex && s.RenderedFrames % 30 == 0) {
    sl::ReflexState reflex{};
    if (s.ReflexGetState(reflex) == sl::Result::eOk &&
        reflex.latencyReportAvailable) {
      for (auto i = sl::kReflexFrameReportCount; i-- > 0;) {
        const auto &report = reflex.frameReport[i];
        if (report.simStartTime &&
            report.gpuRenderEndTime > report.simStartTime) {
          s.HasLatency = true;
          s.RenderLatency = static_cast<double>(report.gpuRenderEndTime -
                                                report.simStartTime) /
                            1000000.0;
          break;
        }
      }
    }
  }
}

}  // namespace core
}  // namespace MGDF
