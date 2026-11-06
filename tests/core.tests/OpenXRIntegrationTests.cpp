#include "stdafx.h"

#include <filesystem>
#include <numeric>

#include "../../src/core/MGDFD3D11RenderBackend.hpp"
#include "../../src/core/MGDFD3D12RenderBackend.hpp"
#include "../../src/core/MGDFD3DAppFramework.hpp"
#include "../../src/core/common/MGDFPreferenceSet.hpp"
#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/core.impl/MGDFRenderSettingsManagerImpl.hpp"
#include "../openxr.runtime/Control.hpp"

using namespace MGDF;
using namespace MGDF::core;

namespace {
class Runtime {
 public:
  Runtime() {
    Resources::Instance(GetModuleHandleW(nullptr));
    wchar_t filename[MAX_PATH]{};
    GetModuleFileNameW(nullptr, filename, MAX_PATH);
    const auto directory = std::filesystem::path(filename).parent_path();
    const auto manifest = (directory / L"openxr.runtime.json").string();
    char *old = nullptr;
    size_t length = 0;
    _dupenv_s(&old, &length, "XR_RUNTIME_JSON");
    if (old) Previous = old;
    free(old);
    _putenv_s("XR_RUNTIME_JSON", manifest.c_str());
    Library = LoadLibraryW((directory / L"openxr.runtime.dll").c_str());
    if (!Library) return;
    Configure =
        reinterpret_cast<FakeXRConfigure>(GetProcAddress(Library, "Configure"));
    Push =
        reinterpret_cast<FakeXRPushState>(GetProcAddress(Library, "PushState"));
    ShouldRender = reinterpret_cast<FakeXRShouldRender>(
        GetProcAddress(Library, "SetShouldRender"));
    Read =
        reinterpret_cast<FakeXRGetStats>(GetProcAddress(Library, "GetStats"));
    LoseInstance = reinterpret_cast<FakeXRInstanceLoss>(
        GetProcAddress(Library, "LoseInstance"));
  }
  ~Runtime() {
    _putenv_s("XR_RUNTIME_JSON", Previous.c_str());
    if (Library) FreeLibrary(Library);
  }
  FakeXRStats Stats() const {
    FakeXRStats result{};
    Read(&result);
    return result;
  }
  HMODULE Library = nullptr;
  FakeXRConfigure Configure = nullptr;
  FakeXRPushState Push = nullptr;
  FakeXRShouldRender ShouldRender = nullptr;
  FakeXRGetStats Read = nullptr;
  FakeXRInstanceLoss LoseInstance = nullptr;

 private:
  std::string Previous;
};

LUID TestAdapter() {
  ComObject<IDXGIFactory6> factory;
  if (FAILED(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.Assign())))) return {};
  ComObject<ID3D12Device10> device;
  if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0,
                               IID_PPV_ARGS(device.Assign())))) {
    ComObject<IDXGIAdapter> warp;
    if (FAILED(factory->EnumWarpAdapter(IID_PPV_ARGS(warp.Assign()))) ||
        FAILED(D3D12CreateDevice(warp, D3D_FEATURE_LEVEL_11_0,
                                 IID_PPV_ARGS(device.Assign()))))
      return {};
  }
  return device->GetAdapterLuid();
}

class FrameLoop final : public D3DAppFramework {
 public:
  FrameLoop(MGDFGraphicsAPI api, bool moduleVR, bool saved = false)
      : D3DAppFramework(GetModuleHandleW(nullptr)),
        API(api),
        Settings(MakeCom<RenderSettingsManager>()) {
    // OpenXR lifecycle tests do not need the separate vendor upscaler runtime.
    _pluginDirectory = std::filesystem::temp_directory_path() /
                       ("mgdf-openxr-" + std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(_pluginDirectory);
    Settings->InitVR(moduleVR, api, saved, false);
  }
  ~FrameLoop() {
    RTWaitForGpuIdle();
    ReleaseDevice();
    std::error_code error;
    std::filesystem::remove_all(_pluginDirectory, error);
  }
  void RequestVR(bool enabled) {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    Settings->CreatePendingSettingsChange(change.Assign());
    EXPECT_TRUE(change->SetVR(enabled));
  }
  void Execute() {
    Deadline = GetTickCount64() + 15000;
    InitWindow("MGDF OpenXR integration test");
    Run();
  }
  std::function<bool(FrameLoop &)> Step;
  IRenderBackend *Backend = nullptr;
  const MGDFGraphicsAPI API;
  ComObject<RenderSettingsManager> Settings;
  std::vector<UINT32> Views;
  UINT32 DeviceResets = 0;
  UINT32 BeforeChanges = 0;
  UINT32 AfterChanges = 0;
  std::atomic<UINT32> FatalErrors{0};
  bool TimedOut = false;

 protected:
  std::unique_ptr<IRenderBackend> CreateRenderBackend(
      const ComObject<IDXGIFactory6> &factory) override {
    const auto fail = [this](const char *sender, const char *message) {
      FatalError(sender, message);
    };
    if (API == MGDF_GRAPHICS_API_D3D12) {
      const MGDFGraphicsRequirements requirements{
          1, {MGDF_GRAPHICS_API_D3D12}, D3D_FEATURE_LEVEL_11_0};
      return std::make_unique<D3D12RenderBackend>(
          factory, requirements, fail, true,
          _pluginDirectory.wstring() + L"\\");
    }
    return std::make_unique<D3D11RenderBackend>(
        factory, GetD3D11FeatureLevels(D3D_FEATURE_LEVEL_11_0), fail, true);
  }
  std::pair<DXGI_FORMAT, DXGI_FORMAT> RTOnBeforeEnumerateDisplayModes()
      override {
    return {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT};
  }
  void RTOnBeforeFirstDraw() override {}
  void RTOnBeforeDeviceReset() override {
    RTOnBeforeBackBufferChange();
    ReleaseDevice();
    QueueResetDevice();
  }
  void RTOnDeviceReset() override { ++DeviceResets; }
  void RTOnInitDevice(IRenderBackend &backend) override {
    Backend = &backend;
    if (API == MGDF_GRAPHICS_API_D3D12) {
      Settings->InitD3D12();
      Device12 = backend.RTGetDevice().As<ID3D12Device>();
      const D3D12_DESCRIPTOR_HEAP_DESC heap{
          .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV, .NumDescriptors = 4};
      EXPECT_EQ(S_OK, Device12->CreateDescriptorHeap(
                          &heap, IID_PPV_ARGS(RTVs.Assign())));
      for (UINT32 i = 0; i < 2; ++i) {
        EXPECT_EQ(S_OK, Device12->CreateCommandAllocator(
                            D3D12_COMMAND_LIST_TYPE_DIRECT,
                            IID_PPV_ARGS(Allocators[i].Assign())));
        EXPECT_EQ(S_OK, Device12->CreateCommandList(
                            0, D3D12_COMMAND_LIST_TYPE_DIRECT, Allocators[i],
                            nullptr, IID_PPV_ARGS(Lists[i].Assign())));
        EXPECT_EQ(S_OK, Lists[i]->Close());
      }
    } else {
      Device11 = backend.RTGetDevice().As<ID3D11Device>();
      Settings->InitFromDevice(Device11);
    }
    Settings->SetGraphicsSupport(backend.RTGetGraphicsSupport());
  }
  void RTOnBeforeBackBufferChange() override { ++BeforeChanges; }
  void RTOnBackBufferChange(IRenderBackend &backend) override {
    ++AfterChanges;
    EXPECT_EQ(backend.VR().HasSession() ? 48U : 0U, backend.VR().View(0).Width);
    EXPECT_EQ(backend.VR().HasSession() ? 64U : 0U, backend.VR().View(1).Width);
  }
  MGDFFullScreenDesc RTOnResetSwapChain(
      DXGI_SWAP_CHAIN_DESC1 &desc, DXGI_SWAP_CHAIN_FULLSCREEN_DESC &fullscreen,
      DXGI_COLOR_SPACE_TYPE &color, const RECT &size) override {
    Settings->OnResetSwapChain(desc, fullscreen, color, size);
    return {};
  }
  std::optional<UINT> RTGetMaxFrameLatency() const override { return 1; }
  UINT32 RTGetFrameGeneration() const override {
    return Settings->GetFrameGeneration();
  }
  MGDFLowLatencyMode RTGetLowLatencyMode() const override {
    return Settings->GetCurrentLowLatencyMode();
  }
  void RTOnDisplayChange(const DXGI_OUTPUT_DESC1 &, UINT dpi, ULONG,
                         const std::vector<DXGI_MODE_DESC1> &) override {
    Settings->SetOutputProperties({.Width = 800, .Height = 600}, dpi,
                                  {{800, 600, 60, 1, FALSE, TRUE}});
  }
  void RTOnResize(UINT32 width, UINT32 height) override {
    Settings->OnResize(width, height);
  }
  bool RTVSyncEnabled() const override { return true; }
  bool RTOnDraw() override {
    const UINT32 count = Backend->VR().ViewCount();
    Views.push_back(count);
    if (count) {
      EXPECT_EQ(2U, count);
      EXPECT_TRUE(Settings->GetVR());
      EXPECT_FLOAT_EQ(1.0f, Backend->VR().View(0).Position[2]);
      EXPECT_FLOAT_EQ(-0.2f, Backend->VR().View(0).Orientation[1]);
      EXPECT_EQ(48U, Backend->VR().View(0).Width);
      EXPECT_EQ(64U, Backend->VR().View(1).Width);
      EXPECT_EQ(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, Backend->VR().View(0).Format);
      DrawEyes();
    }
    Backend->RTClear();
    if (!Done && Step) Done = Step(*this);
    return true;
  }
  void RTOnAfterPresent() override {
    if (Done) CloseWindow();
  }
  bool RTIsBackBufferChangePending() override {
    return Settings->IsBackBufferChangePending();
  }
  std::optional<bool> RTTakeVRRequest() override {
    return Settings->TakeVRRequest();
  }
  void RTOnVRState(bool active) override { Settings->SetVRState(active); }
  MGDFFullScreenDesc RTGetFullscreen() override { return {}; }
  void RTSetFullscreen(const MGDFFullScreenDesc &) override {}
  std::optional<std::pair<UINT32, UINT32>> RTTakeWindowSize() override {
    return std::nullopt;
  }
  bool OnWindowAspectLock() override { return false; }
  void STOnUpdateSim() override {
    if (GetTickCount64() >= Deadline) {
      TimedOut = true;
      CloseWindow();
    }
    SwitchToThread();
  }
  bool OnInitWindow(RECT &size) override {
    size = {0, 0, 800, 600};
    return true;
  }
  void OnExternalClose() override { CloseWindow(); }
  void OnMouseInput(INT32, INT32) override {}
  void OnRawInput(std::function<RAWINPUT *()>) override {}
  LRESULT OnHandleMessage(HWND window, UINT32 message, WPARAM wparam,
                          LPARAM lparam) override {
    return DefWindowProc(window, message, wparam, lparam);
  }
  void OnMoveWindow(INT32, INT32) override {}
  bool OnSetCursor() override { return false; }
  void FatalError(const char *sender, const char *message) override {
    ++FatalErrors;
    ADD_FAILURE() << sender << ": " << message;
    CloseWindow();
  }

 private:
  std::filesystem::path _pluginDirectory;
  void DrawEyes() {
    const float color[4]{0.2f, 0.4f, 0.6f, 1};
    if (Device11) {
      ComObject<ID3D11DeviceContext> context;
      Device11->GetImmediateContext(context.Assign());
      for (UINT32 i = 0; i < 2; ++i) {
        auto texture = Backend->VR().Target(i).As<ID3D11Texture2D>();
        ComObject<ID3D11RenderTargetView> rtv;
        ASSERT_EQ(S_OK, Device11->CreateRenderTargetView(texture, nullptr,
                                                         rtv.Assign()));
        context->ClearRenderTargetView(rtv, color);
      }
    } else {
      const auto slot = Backend->RTGetCurrentFrame().FrameSlot;
      ASSERT_EQ(S_OK, Allocators[slot]->Reset());
      ASSERT_EQ(S_OK, Lists[slot]->Reset(Allocators[slot], nullptr));
      for (UINT32 i = 0; i < 2; ++i) {
        auto texture = Backend->VR().Target(i).As<ID3D12Resource>();
        auto handle = RTVs->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += (static_cast<SIZE_T>(slot) * 2 + i) *
                      Device12->GetDescriptorHandleIncrementSize(
                          D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        Device12->CreateRenderTargetView(texture, nullptr, handle);
        Lists[slot]->ClearRenderTargetView(handle, color, 0, nullptr);
      }
      ASSERT_EQ(S_OK, Lists[slot]->Close());
      ID3D12CommandList *lists[]{Lists[slot]};
      Backend->RTGetQueue(D3D12_COMMAND_LIST_TYPE_DIRECT)
          ->ExecuteCommandLists(1, lists);
    }
  }
  void ReleaseDevice() {
    for (auto &list : Lists) list.Clear();
    for (auto &allocator : Allocators) allocator.Clear();
    RTVs.Clear();
    Device12.Clear();
    Device11.Clear();
  }
  ULONGLONG Deadline = 0;
  bool Done = false;
  ComObject<ID3D11Device> Device11;
  ComObject<ID3D12Device> Device12;
  ComObject<ID3D12DescriptorHeap> RTVs;
  std::array<ComObject<ID3D12CommandAllocator>, 2> Allocators;
  std::array<ComObject<ID3D12GraphicsCommandList>, 2> Lists;
};

class OpenXRIntegrationTests : public testing::TestWithParam<MGDFGraphicsAPI> {
};

TEST_P(OpenXRIntegrationTests, HostRendersTwoEyesSkipsFramesAndTogglesVR) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0;
  UINT32 expectedImages = 0;
  {
    FrameLoop loop(GetParam(), true);
    ASSERT_TRUE(loop.Settings->GetVRSupport());
    EXPECT_EQ(0U, runtime.Stats().InstancesCreated);
    loop.Step = [&](FrameLoop &host) {
      const auto count = host.Backend->VR().ViewCount();
      if (phase == 0) {
        EXPECT_EQ(0U, count);
        host.RequestVR(true);
        ++phase;
      } else if (phase == 1 && count == 2 &&
                 runtime.Stats().ProjectionFrames >= 3) {
        EXPECT_TRUE(host.Backend->VR().Focused());
        runtime.ShouldRender(false);
        ++phase;
      } else if (phase == 2 && !count) {
        EXPECT_TRUE(host.Settings->GetVR());
        runtime.ShouldRender(true);
        runtime.Push(XR_SESSION_STATE_VISIBLE);
        ++phase;
      } else if (phase == 3 && count == 2) {
        EXPECT_FALSE(host.Backend->VR().Focused());
        host.RequestVR(false);
        ++phase;
      } else if (phase == 4 && !host.Settings->GetVR()) {
        EXPECT_EQ(0U, count);
        host.RequestVR(true);
        ++phase;
      } else if (phase == 5 && count == 2) {
        runtime.Push(XR_SESSION_STATE_STOPPING);
        runtime.Push(XR_SESSION_STATE_IDLE);
        runtime.Push(XR_SESSION_STATE_EXITING);
        ++phase;
      } else if (phase == 6 && !host.Settings->GetVR()) {
        EXPECT_EQ(0U, count);
        ++phase;
        return true;
      }
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
    EXPECT_EQ(7U, phase);
    expectedImages = std::accumulate(loop.Views.begin(), loop.Views.end(), 0U);
    EXPECT_EQ(loop.BeforeChanges, loop.AfterChanges);
    EXPECT_GE(loop.AfterChanges, 4U);
  }
  const auto stats = runtime.Stats();
  EXPECT_EQ(expectedImages, stats.SubmittedImages);
  EXPECT_EQ(2U, stats.SessionsCreated);
  EXPECT_EQ(1U, stats.ExitRequests);
  EXPECT_EQ(2U, stats.SessionsEnded);
  EXPECT_GE(stats.EmptyFrames, 1U);
  EXPECT_EQ(0U,
            stats.LiveInstances + stats.LiveSessions + stats.LiveSwapchains);
  EXPECT_EQ(0U, stats.Errors);
}

TEST_P(OpenXRIntegrationTests, IdleSessionResumesWithoutChangingEyeTargets) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0;
  UINT32 changes = 0;
  UINT32 swapchains = 0;
  UINT32 idleFrames = 0;
  UINT32 completedFrames = 0;
  {
    FrameLoop loop(GetParam(), true, true);
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      const auto stats = runtime.Stats();
      if (phase > 0 && phase < 5) {
        EXPECT_TRUE(host.Settings->GetVR());
        EXPECT_TRUE(vr.HasSession());
        EXPECT_FALSE(vr.NeedsFallback());
        EXPECT_EQ(changes, host.BeforeChanges);
        EXPECT_EQ(changes, host.AfterChanges);
        EXPECT_EQ(swapchains, stats.SwapchainsCreated);
        EXPECT_EQ(1U, stats.SessionsCreated);
        EXPECT_EQ(0U, host.Settings->GetMaxGeneratedFrames());
        EXPECT_EQ(0U, host.Settings->GetFrameGeneration());
        EXPECT_EQ(MGDF_LOW_LATENCY_OFF,
                  host.Settings->GetCurrentLowLatencyMode());
      }
      if (phase == 0 && vr.ViewCount() == 2) {
        EXPECT_TRUE(vr.Focused());
        changes = host.AfterChanges;
        swapchains = stats.SwapchainsCreated;
        host.Settings->SetGraphicsSupport({.MaxGeneratedFrames = 3});
        runtime.Push(XR_SESSION_STATE_STOPPING);
        ++phase;
      } else if (phase == 1) {
        EXPECT_FALSE(vr.Running());
        EXPECT_FALSE(vr.Focused());
        EXPECT_EQ(0U, vr.ViewCount());
        EXPECT_EQ(1U, stats.SessionsEnded);
        completedFrames = stats.Frames;
        runtime.Push(XR_SESSION_STATE_IDLE);
        ++phase;
      } else if (phase == 2) {
        EXPECT_FALSE(vr.Running());
        EXPECT_EQ(0U, vr.ViewCount());
        EXPECT_EQ(completedFrames, stats.Frames);
        if (++idleFrames == 3) {
          runtime.Push(XR_SESSION_STATE_READY);
          runtime.Push(XR_SESSION_STATE_FOCUSED);
          ++phase;
        }
      } else if (phase == 3 && vr.ViewCount() == 2) {
        EXPECT_TRUE(vr.Focused());
        EXPECT_EQ(2U, stats.SessionsBegun);
        runtime.Push(XR_SESSION_STATE_STOPPING);
        runtime.Push(XR_SESSION_STATE_IDLE);
        ++phase;
      } else if (phase == 4) {
        EXPECT_FALSE(vr.Running());
        EXPECT_EQ(0U, vr.ViewCount());
        host.RequestVR(false);
        ++phase;
      } else if (phase == 5 && !host.Settings->GetVR()) {
        EXPECT_FALSE(vr.HasSession());
        EXPECT_GT(host.AfterChanges, changes);
        ++phase;
        return true;
      }
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_EQ(6U, phase);
  EXPECT_EQ(2U, runtime.Stats().SessionsEnded);
  EXPECT_EQ(0U, runtime.Stats().ExitRequests);
  EXPECT_EQ(0U, runtime.Stats().Errors);
  EXPECT_EQ(0U, runtime.Stats().LiveSwapchains + runtime.Stats().LiveSessions +
                    runtime.Stats().LiveInstances);
}

TEST_P(OpenXRIntegrationTests, MissingHeadsetKeepsWindowAndSavedPreference) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  const FakeXRConfig config{TestAdapter(), true};
  runtime.Configure(&config);
  {
    FrameLoop loop(GetParam(), true, true);
    loop.Step = [](auto &host) { return host.Views.size() >= 3; };
    loop.Execute();
    EXPECT_EQ(0U, loop.FatalErrors.load());
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_FALSE(loop.Settings->GetVR());
    EXPECT_TRUE(
        std::ranges::all_of(loop.Views, [](auto views) { return views == 0; }));
    ComObject<IMGDFPreferenceSet> preferences;
    loop.Settings->GetPreferences(preferences.Assign());
    EXPECT_EQ("1", static_cast<PreferenceSetImpl *>(preferences.Get())
                       ->Preferences.at("host.vr"));
  }
  EXPECT_EQ(1U, runtime.Stats().InstancesCreated);
  EXPECT_EQ(0U, runtime.Stats().SessionsCreated);
  EXPECT_EQ(0U, runtime.Stats().LiveInstances);
}

TEST_P(OpenXRIntegrationTests, ModuleWithoutVRNeverStartsRuntimeOrSession) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  {
    FrameLoop loop(GetParam(), false, true);
    EXPECT_FALSE(loop.Settings->GetVRSupport());
    ComObject<IMGDFPendingRenderSettingsChange> change;
    loop.Settings->CreatePendingSettingsChange(change.Assign());
    EXPECT_FALSE(change->SetVR(TRUE));
    change.Clear();
    loop.Step = [](auto &host) { return host.Views.size() >= 3; };
    loop.Execute();
    EXPECT_FALSE(loop.Settings->GetVR());
    EXPECT_EQ(0U, loop.FatalErrors.load());
    EXPECT_FALSE(loop.TimedOut);
  }
  EXPECT_EQ(0U, runtime.Stats().InstancesCreated);
}

TEST_P(OpenXRIntegrationTests, RuntimeLossReturnsToWindow) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  for (const auto state :
       {XR_SESSION_STATE_LOSS_PENDING, XR_SESSION_STATE_EXITING,
        XR_SESSION_STATE_UNKNOWN}) {
    const FakeXRConfig config{TestAdapter()};
    runtime.Configure(&config);
    bool sent = false;
    {
      FrameLoop loop(GetParam(), true, true);
      loop.Step = [&](auto &host) {
        if (!sent && host.Backend->VR().ViewCount() == 2) {
          if (state == XR_SESSION_STATE_UNKNOWN)
            runtime.LoseInstance();
          else
            runtime.Push(state);
          sent = true;
        } else if (sent && !host.Settings->GetVR())
          return true;
        return false;
      };
      loop.Execute();
      EXPECT_TRUE(sent);
      EXPECT_FALSE(loop.TimedOut);
      EXPECT_EQ(0U, loop.FatalErrors.load());
      EXPECT_FALSE(loop.Settings->GetVR());
    }
    EXPECT_EQ(0U, runtime.Stats().LiveInstances + runtime.Stats().LiveSessions +
                      runtime.Stats().LiveSwapchains);
    EXPECT_EQ(0U, runtime.Stats().Errors);
  }
}

TEST_P(OpenXRIntegrationTests, SwitchesToTheRuntimeAdapterThroughDeviceReset) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  ComObject<IDXGIFactory6> factory;
  ASSERT_EQ(S_OK, CreateDXGIFactory2(0, IID_PPV_ARGS(factory.Assign())));
  ComObject<IDXGIAdapter> warp;
  ASSERT_EQ(S_OK, factory->EnumWarpAdapter(IID_PPV_ARGS(warp.Assign())));
  DXGI_ADAPTER_DESC desc{};
  ASSERT_EQ(S_OK, warp->GetDesc(&desc));
  const FakeXRConfig config{desc.AdapterLuid};
  runtime.Configure(&config);
  {
    FrameLoop loop(GetParam(), true);
    bool requested = false;
    bool needsReset = false;
    loop.Step = [&](auto &host) {
      if (!requested) {
        DXGI_ADAPTER_DESC initial{};
        host.Backend->RTGetAdapter()->GetDesc(&initial);
        needsReset = initial.AdapterLuid.LowPart != config.Adapter.LowPart ||
                     initial.AdapterLuid.HighPart != config.Adapter.HighPart;
        host.RequestVR(true);
        requested = true;
      } else if (host.Backend->VR().ViewCount() == 2)
        return true;
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
    EXPECT_EQ(needsReset ? 1U : 0U, loop.DeviceResets);
    DXGI_ADAPTER_DESC actual{};
    loop.Backend->RTGetAdapter()->GetDesc(&actual);
    EXPECT_EQ(config.Adapter.LowPart, actual.AdapterLuid.LowPart);
    EXPECT_EQ(config.Adapter.HighPart, actual.AdapterLuid.HighPart);
  }
  EXPECT_GE(runtime.Stats().SubmittedImages, 2U);
  EXPECT_EQ(0U, runtime.Stats().Errors);
}

TEST_P(OpenXRIntegrationTests, CommandLineAndSessionStateKeepSavedPreferences) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  auto settings = MakeCom<RenderSettingsManager>();
  settings->InitVR(true, GetParam(), false, true);
  EXPECT_TRUE(settings->GetVRSupport());
  EXPECT_EQ(std::optional<bool>(true), settings->TakeVRRequest());
  EXPECT_EQ(0U, runtime.Stats().InstancesCreated);
  settings->SetGraphicsSupport(
      {.MaxGeneratedFrames = 3,
       .LowLatencyModes = {MGDF_LOW_LATENCY_OFF, MGDF_LOW_LATENCY_BOOST}});
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    ASSERT_TRUE(change->SetFrameGeneration(2));
    ASSERT_TRUE(change->SetLowLatencyMode(MGDF_LOW_LATENCY_BOOST));
  }
  settings->SetVRState(true);
  EXPECT_EQ(0U, settings->GetMaxGeneratedFrames());
  EXPECT_EQ(0U, settings->GetFrameGeneration());
  EXPECT_EQ(MGDF_LOW_LATENCY_OFF, settings->GetCurrentLowLatencyMode());
  ComObject<IMGDFPreferenceSet> preferences;
  settings->GetPreferences(preferences.Assign());
  const auto &saved =
      static_cast<PreferenceSetImpl *>(preferences.Get())->Preferences;
  EXPECT_EQ("0", saved.at("host.vr"));
  EXPECT_EQ("2", saved.at("host.frameGeneration"));
  settings->SetVRState(false);
  EXPECT_EQ(3U, settings->GetMaxGeneratedFrames());
  EXPECT_EQ(2U, settings->GetFrameGeneration());
  EXPECT_EQ(MGDF_LOW_LATENCY_BOOST, settings->GetCurrentLowLatencyMode());
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    ASSERT_TRUE(change->SetVR(TRUE));
    change->Cancel();
  }
  EXPECT_FALSE(settings->TakeVRRequest());
}

TEST_P(OpenXRIntegrationTests,
       SuccessfulAcquireAndWaitStatusesKeepImageOwnership) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  const FakeXRConfig config{TestAdapter(), false, true, true, true};
  runtime.Configure(&config);
  bool rendered = false;
  {
    FrameLoop loop(GetParam(), true, true);
    loop.Step = [&](auto &host) {
      if (host.Backend->VR().ViewCount() == 2) rendered = true;
      return rendered && !host.Settings->GetVR();
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_TRUE(rendered);
  EXPECT_EQ(2U, runtime.Stats().SubmittedImages);
  EXPECT_EQ(0U, runtime.Stats().Errors);
  EXPECT_EQ(0U, runtime.Stats().LiveSwapchains + runtime.Stats().LiveSessions +
                    runtime.Stats().LiveInstances);
}

INSTANTIATE_TEST_SUITE_P(Backends, OpenXRIntegrationTests,
                         testing::Values(MGDF_GRAPHICS_API_D3D11,
                                         MGDF_GRAPHICS_API_D3D12));
}  // namespace
