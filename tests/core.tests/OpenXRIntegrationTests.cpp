#include "stdafx.h"

#include <cmath>
#include <filesystem>
#include <numeric>

#include "../../src/core/MGDFD3D11RenderBackend.hpp"
#include "../../src/core/MGDFD3D12RenderBackend.hpp"
#include "../../src/core/MGDFD3DAppFramework.hpp"
#include "../../src/core/common/MGDFPreferenceSet.hpp"
#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/common/MGDFVersionHelper.hpp"
#include "../../src/core/common/MGDFVersionInfo.hpp"
#include "../../src/core/core.impl/MGDFRenderSettingsManagerImpl.hpp"
#include "../../src/core/input/xinput/XInputManagerComponent.hpp"
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
    ChangeMask = reinterpret_cast<FakeXRChangeMask>(
        GetProcAddress(Library, "ChangeMask"));
    FailAcquire = reinterpret_cast<FakeXRFailAcquire>(
        GetProcAddress(Library, "FailAcquire"));
    SetInput =
        reinterpret_cast<FakeXRSetInput>(GetProcAddress(Library, "SetInput"));
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
  FakeXRChangeMask ChangeMask = nullptr;
  FakeXRShouldRender ShouldRender = nullptr;
  FakeXRGetStats Read = nullptr;
  FakeXRInstanceLoss LoseInstance = nullptr;
  FakeXRSetInput SetInput = nullptr;
  FakeXRFailAcquire FailAcquire = nullptr;

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
  std::string ApplicationName = "MGDF OpenXR integration test";
  IRenderBackend *Backend = nullptr;
  ComObject<input::IInputManagerComponent> Input;
  const MGDFGraphicsAPI API;
  ComObject<RenderSettingsManager> Settings;
  std::vector<UINT32> Views;
  UINT32 DeviceResets = 0;
  UINT32 BeforeChanges = 0;
  UINT32 AfterChanges = 0;
  std::atomic<UINT32> FatalErrors{0};
  bool TimedOut = false;

 protected:
  const char *RTGetApplicationName() const override {
    return ApplicationName.c_str();
  }
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
    if (Input) backend.VR().SetInputState(Input->GetVRInputState());
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

TEST_P(OpenXRIntegrationTests, InstanceIdentifiesTheGameAndEngine) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.Configure);
  const std::array<std::pair<std::string, std::string>, 2> names{{
      {"Junkship \xc3\xa9", "Junkship \xc3\xa9"},
      {std::string(126, 'x') + "\xc3\xa9", std::string(126, 'x')},
  }};
  for (const auto &[name, expected] : names) {
    const FakeXRConfig config{TestAdapter()};
    runtime.Configure(&config);
    {
      FrameLoop loop(GetParam(), true, true);
      loop.ApplicationName = name;
      loop.Step = [](auto &host) {
        return host.Backend->VR().ViewCount() == 2;
      };
      loop.Execute();
      EXPECT_FALSE(loop.TimedOut);
      EXPECT_EQ(0U, loop.FatalErrors.load());
    }
    const auto info = runtime.Stats().ApplicationInfo;
    EXPECT_STREQ(expected.c_str(), info.applicationName);
    EXPECT_STREQ("MGDF", info.engineName);
    const auto version = VersionHelper::Create(MGDFVersionInfo::MGDF_VERSION());
    EXPECT_EQ(version.Major, info.engineVersion >> 24);
    EXPECT_EQ(version.Minor, (info.engineVersion >> 16) & 255);
    EXPECT_EQ((std::max)(0, version.Build), (info.engineVersion >> 8) & 255);
    EXPECT_EQ((std::max)(0, version.Revision), info.engineVersion & 255);
    EXPECT_EQ(0U, runtime.Stats().Errors);
  }
}

TEST_P(OpenXRIntegrationTests, ViewCallsReportInvalidArgumentsAndSessionState) {
  Runtime runtime;
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0;
  {
    FrameLoop loop(GetParam(), true);
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      MGDFVREye view{};
      ComObject<IUnknown> image;
      EXPECT_EQ(E_INVALIDARG, vr.GetView(2, &view));
      EXPECT_EQ(E_INVALIDARG, vr.GetView(0, nullptr));
      EXPECT_EQ(E_INVALIDARG, vr.GetTarget(2, image));
      if (phase == 0) {
        EXPECT_EQ(E_ILLEGAL_METHOD_CALL, vr.GetView(0, &view));
        EXPECT_EQ(0U, view.Width);
        EXPECT_EQ(E_ILLEGAL_METHOD_CALL, vr.GetTarget(0, image));
        EXPECT_FALSE(image);
        host.RequestVR(true);
        ++phase;
      } else if (phase == 1 && vr.ViewCount() == 2) {
        EXPECT_EQ(S_OK, vr.GetView(0, &view));
        EXPECT_EQ(48U, view.Width);
        EXPECT_EQ(S_OK, vr.GetTarget(0, image));
        EXPECT_TRUE(image);
        std::thread wrongThread([&] {
          MGDFVREye wrong{};
          ComObject<IUnknown> target;
          EXPECT_EQ(E_ILLEGAL_METHOD_CALL, vr.GetView(0, &wrong));
          EXPECT_EQ(E_ILLEGAL_METHOD_CALL, vr.GetTarget(0, target));
          EXPECT_FALSE(target);
        });
        wrongThread.join();
        runtime.ShouldRender(false);
        ++phase;
      } else if (phase == 2 && !vr.ViewCount()) {
        EXPECT_EQ(E_ILLEGAL_METHOD_CALL, vr.GetTarget(0, image));
        EXPECT_FALSE(image);
        ++phase;
        return true;
      }
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_EQ(3U, phase);
  EXPECT_EQ(0U, runtime.Stats().Errors);
}

TEST_P(OpenXRIntegrationTests,
       HiddenMaskIsCachedAndChangesOnlyWithItsRevision) {
  Runtime runtime;
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0, reads = 0;
  UINT64 version = 0;
  {
    FrameLoop loop(GetParam(), true, true);
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      UINT32 vertices = 0, count = 0;
      UINT64 current = 0;
      if (!vr.ViewCount()) return false;
      EXPECT_EQ(E_INVALIDARG,
                vr.GetVREyeMask(2, &vertices, nullptr, &count, nullptr));
      EXPECT_EQ(E_INVALIDARG,
                vr.GetVREyeMask(0, nullptr, nullptr, &count, nullptr));
      EXPECT_EQ(E_INVALIDARG, vr.GetVREyeMaskVersion(nullptr));
      if (phase < 3) {
        EXPECT_EQ(S_OK, vr.GetVREyeMaskVersion(&current));
        EXPECT_EQ(S_OK,
                  vr.GetVREyeMask(0, &vertices, nullptr, &count, nullptr));
        EXPECT_EQ(3U, vertices);
        EXPECT_EQ(3U, count);
        FLOAT xy[6]{};
        UINT32 indices[3]{};
        vertices = 2;
        EXPECT_EQ(E_INVALIDARG,
                  vr.GetVREyeMask(0, &vertices, xy, &count, indices));
        EXPECT_EQ(3U, vertices);
        EXPECT_EQ(S_OK, vr.GetVREyeMask(0, &vertices, xy, &count, indices));
        EXPECT_FLOAT_EQ(phase == 2 ? -0.7f : -0.8f, xy[0]);
        EXPECT_FLOAT_EQ(-0.6f, xy[1]);
        EXPECT_FLOAT_EQ(0.4f, xy[5]);
        EXPECT_EQ(0U, indices[0]);
        EXPECT_EQ(1U, indices[1]);
        EXPECT_EQ(2U, indices[2]);
        EXPECT_GT((xy[2] - xy[0]) * (xy[5] - xy[1]) -
                      (xy[3] - xy[1]) * (xy[4] - xy[0]),
                  0);
      }
      if (phase == 0) {
        version = current;
        reads = runtime.Stats().MaskReads;
        ++phase;
      } else if (phase == 1) {
        EXPECT_EQ(version, current);
        EXPECT_EQ(reads, runtime.Stats().MaskReads);
        runtime.ChangeMask(1);
        ++phase;
      } else if (phase == 2) {
        EXPECT_NE(version, current);
        EXPECT_GT(runtime.Stats().MaskReads, reads);
        runtime.ChangeMask(2);
        ++phase;
      } else if (phase == 3) {
        EXPECT_EQ(S_FALSE,
                  vr.GetVREyeMask(0, &vertices, nullptr, &count, nullptr));
        EXPECT_EQ(0U, vertices);
        EXPECT_EQ(0U, count);
        runtime.ChangeMask(3);
        ++phase;
      } else if (phase == 4) {
        EXPECT_EQ(E_FAIL,
                  vr.GetVREyeMask(0, &vertices, nullptr, &count, nullptr));
        EXPECT_EQ(E_FAIL, vr.GetVREyeMaskVersion(&current));
        EXPECT_EQ(0U, vertices);
        EXPECT_EQ(0U, current);
        ++phase;
        return true;
      }
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_EQ(5U, phase);
  EXPECT_EQ(0U, runtime.Stats().Errors);
}

TEST_P(OpenXRIntegrationTests,
       DepthIsSubmittedOnlyWhenBothEyesHaveCurrentRanges) {
  Runtime runtime;
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0;
  {
    FrameLoop loop(GetParam(), true, true);
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      if (!vr.ViewCount()) return false;
      ComObject<IUnknown> target;
      EXPECT_EQ(DXGI_FORMAT_D32_FLOAT, vr.View(0).DepthFormat);
      EXPECT_EQ(S_OK, vr.GetDepthTarget(0, target));
      EXPECT_TRUE(target);
      EXPECT_EQ(E_INVALIDARG, vr.GetDepthTarget(2, target));
      EXPECT_FALSE(target);
      EXPECT_EQ(E_INVALIDARG, vr.SetVREyeDepthRange(2, 1, 10));
      EXPECT_EQ(E_INVALIDARG, vr.SetVREyeDepthRange(0, 0, 10));
      EXPECT_EQ(E_INVALIDARG, vr.SetVREyeDepthRange(0, 1, 1));
      EXPECT_EQ(E_INVALIDARG, vr.SetVREyeDepthRange(0, NAN, 10));
      if (phase == 0) {
        EXPECT_EQ(S_OK, vr.SetVREyeDepthRange(0, INFINITY, 0.1f));
        EXPECT_EQ(S_OK, vr.SetVREyeDepthRange(1, 0.2f, INFINITY));
      } else if (phase == 1) {
        const auto stats = runtime.Stats();
        EXPECT_NE(0U, stats.LastViewNext[0]);
        EXPECT_NE(0U, stats.LastViewNext[1]);
        EXPECT_EQ(INFINITY, stats.LastDepth[0].nearZ);
        EXPECT_FLOAT_EQ(0.1f, stats.LastDepth[0].farZ);
        EXPECT_EQ(INFINITY, stats.LastDepth[1].farZ);
        EXPECT_EQ(48, stats.LastDepth[0].subImage.imageRect.extent.width);
        EXPECT_EQ(64, stats.LastDepth[1].subImage.imageRect.extent.width);
      } else if (phase == 2) {
        EXPECT_EQ(0U, runtime.Stats().LastViewNext[0]);
        EXPECT_EQ(0U, runtime.Stats().LastViewNext[1]);
        EXPECT_EQ(S_OK, vr.SetVREyeDepthRange(0, 100, 0.1f));
      } else if (phase == 3) {
        EXPECT_EQ(0U, runtime.Stats().LastViewNext[0]);
        EXPECT_EQ(0U, runtime.Stats().LastViewNext[1]);
        ++phase;
        return true;
      }
      ++phase;
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_EQ(4U, phase);
  EXPECT_EQ(0U, runtime.Stats().Errors);
}

TEST_P(OpenXRIntegrationTests, PanelUsesNativePoseSpaceAndDeferredSize) {
  Runtime runtime;
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0, swapchains = 0;
  MGDFVRPanel panel{80, 32,   {0.25f, 0.5f, 2},  {0, 0.6f, 0, 0.8f},
                    1,  0.4f, MGDF_VR_SPACE_VIEW};
  {
    FrameLoop loop(GetParam(), true, true);
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      if (!vr.ViewCount()) return false;
      ComObject<IUnknown> target;
      if (phase == 0) {
        EXPECT_EQ(S_FALSE, vr.GetPanelTarget(target));
        EXPECT_FALSE(target);
        auto invalid = panel;
        invalid.PixelWidth = 0;
        EXPECT_EQ(E_INVALIDARG, vr.SetVRPanel(&invalid));
        EXPECT_EQ(S_OK, vr.SetVRPanel(&panel));
        EXPECT_EQ(S_FALSE, vr.GetPanelTarget(target));
      } else if (phase == 1) {
        EXPECT_EQ(S_OK, vr.GetPanelTarget(target));
        EXPECT_TRUE(target);
      } else if (phase == 2) {
        const auto stats = runtime.Stats();
        swapchains = stats.SwapchainsCreated;
        EXPECT_EQ(2U, stats.LastLayerCount);
        EXPECT_EQ(XR_REFERENCE_SPACE_TYPE_VIEW, stats.PanelSpace);
        EXPECT_FLOAT_EQ(-2, stats.LastQuads[0].pose.position.z);
        EXPECT_FLOAT_EQ(-0.6f, stats.LastQuads[0].pose.orientation.y);
        EXPECT_FLOAT_EQ(0.25f, stats.LastQuads[0].pose.position.x);
        EXPECT_EQ(XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT,
                  stats.LastQuads[0].layerFlags);
        panel.Space = MGDF_VR_SPACE_LOCAL;
        panel.PixelWidth = 96;
        EXPECT_EQ(S_OK, vr.SetVRPanel(&panel));
      } else if (phase == 3) {
        EXPECT_EQ(swapchains + 1, runtime.Stats().SwapchainsCreated);
        EXPECT_EQ(S_OK, vr.GetPanelTarget(target));
      } else if (phase == 4) {
        EXPECT_EQ(XR_REFERENCE_SPACE_TYPE_LOCAL, runtime.Stats().PanelSpace);
        EXPECT_EQ(96,
                  runtime.Stats().LastQuads[0].subImage.imageRect.extent.width);
        EXPECT_EQ(S_OK, vr.SetVRPanel(nullptr));
        EXPECT_EQ(S_FALSE, vr.GetPanelTarget(target));
        EXPECT_FALSE(target);
      } else if (phase == 5) {
        EXPECT_EQ(1U, runtime.Stats().LastLayerCount);
        ++phase;
        return true;
      }
      ++phase;
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_EQ(6U, phase);
  EXPECT_EQ(0U, runtime.Stats().Errors);
}

TEST_P(OpenXRIntegrationTests,
       MissingFeaturesAndCreationFailuresHaveDistinctResults) {
  Runtime runtime;
  for (const bool failure : {false, true}) {
    FakeXRConfig config{TestAdapter()};
    config.MaskExtension = false;
    config.DepthExtension = failure;
    config.PanelSupport = failure;
    config.FailDepth = failure;
    config.FailWidth = failure ? 80 : 0;
    runtime.Configure(&config);
    UINT32 phase = 0;
    {
      FrameLoop loop(GetParam(), true, true);
      loop.Step = [&](auto &host) {
        auto &vr = host.Backend->VR();
        if (!vr.ViewCount()) return false;
        UINT32 vertices = 9, indices = 9;
        UINT64 version = 9;
        EXPECT_EQ(E_NOTIMPL,
                  vr.GetVREyeMask(0, &vertices, nullptr, &indices, nullptr));
        EXPECT_EQ(0U, vertices);
        EXPECT_EQ(0U, indices);
        EXPECT_EQ(E_NOTIMPL, vr.GetVREyeMaskVersion(&version));
        EXPECT_EQ(0U, version);
        ComObject<IUnknown> target;
        EXPECT_EQ(failure ? E_FAIL : E_NOTIMPL, vr.GetDepthTarget(0, target));
        EXPECT_FALSE(target);
        EXPECT_EQ(failure ? E_FAIL : E_NOTIMPL,
                  vr.SetVREyeDepthRange(0, 1, 10));
        EXPECT_EQ(DXGI_FORMAT_UNKNOWN, vr.View(0).DepthFormat);
        if (phase == 0) {
          MGDFVRPanel panel{80, 32, {0, 0, 2},          {0, 0, 0, 1},
                            1,  1,  MGDF_VR_SPACE_LOCAL};
          EXPECT_EQ(failure ? S_OK : E_NOTIMPL, vr.SetVRPanel(&panel));
        } else {
          EXPECT_EQ(failure ? E_FAIL : E_NOTIMPL, vr.GetPanelTarget(target));
          EXPECT_FALSE(target);
          EXPECT_EQ(0U, runtime.Stats().LastViewNext[0]);
          EXPECT_EQ(0U, runtime.Stats().LastViewNext[1]);
          ++phase;
          return true;
        }
        ++phase;
        return false;
      };
      loop.Execute();
      EXPECT_FALSE(loop.TimedOut);
      EXPECT_EQ(0U, loop.FatalErrors.load());
    }
    EXPECT_EQ(2U, phase);
    EXPECT_EQ(0U, runtime.Stats().Errors);
  }
}

TEST_P(OpenXRIntegrationTests,
       ControllersPublishConvertedStateAndDeliverQueuedHaptics) {
  Runtime runtime;
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  FakeXRInput input;
  UINT32 phase = 0;
  {
    FrameLoop loop(GetParam(), true);
    ASSERT_TRUE(input::xinput::CreateInputManagerComponent(loop.Input));
    MGDFVRController disconnected{};
    MGDFVRPose noGaze{};
    EXPECT_FALSE(loop.Input->GetVRController(MGDF_VR_HAND_LEFT, &disconnected));
    EXPECT_FALSE(loop.Input->GetVRGaze(&noGaze));
    EXPECT_FALSE(loop.Input->VibrateVRController(MGDF_VR_HAND_LEFT, 1, 0.1f));
    EXPECT_FALSE(loop.Input->StopVRControllerVibration(MGDF_VR_HAND_LEFT));
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      MGDFVRController controller{};
      MGDFVRPose pose{};
      EXPECT_EQ(E_INVALIDARG,
                vr.GetControllerPose(static_cast<MGDFVRHand>(2),
                                     MGDF_VR_CONTROLLER_GRIP, &pose));
      EXPECT_EQ(E_INVALIDARG,
                vr.GetControllerPose(MGDF_VR_HAND_LEFT, MGDF_VR_CONTROLLER_GRIP,
                                     nullptr));
      EXPECT_EQ(E_INVALIDARG, vr.GetGaze(nullptr));
      EXPECT_FALSE(
          host.Input->GetVRController(static_cast<MGDFVRHand>(2), &controller));
      EXPECT_FALSE(host.Input->GetVRController(MGDF_VR_HAND_LEFT, nullptr));
      EXPECT_FALSE(host.Input->GetVRGaze(nullptr));
      if (phase == 0) {
        EXPECT_EQ(E_ILLEGAL_METHOD_CALL,
                  vr.GetControllerPose(MGDF_VR_HAND_LEFT,
                                       MGDF_VR_CONTROLLER_GRIP, &pose));
        EXPECT_EQ(E_ILLEGAL_METHOD_CALL, vr.GetGaze(&pose));
        host.RequestVR(true);
        ++phase;
      } else if (phase == 1 && vr.ViewCount()) {
        EXPECT_TRUE(
            host.Input->GetVRController(MGDF_VR_HAND_LEFT, &controller));
        EXPECT_EQ(MGDF_VR_PROFILE_TOUCH, controller.Profile);
        EXPECT_TRUE(controller.Grip.PositionValid);
        EXPECT_TRUE(controller.Aim.OrientationValid);
        EXPECT_FLOAT_EQ(0.5f, controller.Grip.Position[2]);
        EXPECT_FLOAT_EQ(-0.6f, controller.Grip.Orientation[1]);
        EXPECT_FLOAT_EQ(0.75f, controller.Trigger);
        EXPECT_FLOAT_EQ(0.4f, controller.Squeeze);
        EXPECT_FLOAT_EQ(0.2f, controller.StickX);
        EXPECT_FLOAT_EQ(-0.3f, controller.StickY);
        EXPECT_EQ(input.Buttons[0], controller.Buttons);
        EXPECT_EQ(S_OK, vr.GetControllerPose(MGDF_VR_HAND_LEFT,
                                             MGDF_VR_CONTROLLER_AIM, &pose));
        EXPECT_FLOAT_EQ(0.6f, pose.Position[2]);
        EXPECT_FLOAT_EQ(-0.6f, pose.Orientation[0]);
        EXPECT_EQ(E_NOTIMPL, vr.GetGaze(&pose));
        EXPECT_FALSE(host.Input->GetVRGaze(&pose));
        EXPECT_FALSE(
            host.Input->VibrateVRController(MGDF_VR_HAND_LEFT, 2, 0.1f));
        EXPECT_FALSE(host.Input->VibrateVRController(MGDF_VR_HAND_LEFT, 1, 0));
        EXPECT_FALSE(
            host.Input->VibrateVRController(MGDF_VR_HAND_LEFT, NAN, 0.1f));
        std::thread sim([&] {
          MGDFVRController right{};
          EXPECT_TRUE(host.Input->GetVRController(MGDF_VR_HAND_RIGHT, &right));
          EXPECT_EQ(input.Buttons[1], right.Buttons);
          EXPECT_TRUE(
              host.Input->VibrateVRController(MGDF_VR_HAND_RIGHT, 0.6f, 0.25f));
          MGDFVRPose wrong{};
          EXPECT_EQ(E_ILLEGAL_METHOD_CALL,
                    vr.GetControllerPose(MGDF_VR_HAND_LEFT,
                                         MGDF_VR_CONTROLLER_GRIP, &wrong));
          EXPECT_EQ(E_ILLEGAL_METHOD_CALL, vr.GetGaze(&wrong));
        });
        sim.join();
        ++phase;
      } else if (phase == 2) {
        const auto stats = runtime.Stats();
        EXPECT_EQ(1U, stats.HapticsApplied);
        EXPECT_EQ(1U, stats.LastHapticHand);
        EXPECT_FLOAT_EQ(0.6f, stats.LastHaptic.amplitude);
        EXPECT_EQ(250000000, stats.LastHaptic.duration);
        EXPECT_TRUE(host.Input->StopVRControllerVibration(MGDF_VR_HAND_RIGHT));
        ++phase;
      } else if (phase == 3) {
        EXPECT_EQ(1U, runtime.Stats().HapticsStopped);
        runtime.Push(XR_SESSION_STATE_VISIBLE);
        ++phase;
      } else if (phase == 4) {
        EXPECT_FALSE(
            host.Input->GetVRController(MGDF_VR_HAND_LEFT, &controller));
        EXPECT_EQ(0U, controller.Buttons);
        EXPECT_FALSE(controller.Grip.PositionValid);
        EXPECT_EQ(S_FALSE,
                  vr.GetControllerPose(MGDF_VR_HAND_LEFT,
                                       MGDF_VR_CONTROLLER_GRIP, &pose));
        EXPECT_FALSE(
            host.Input->VibrateVRController(MGDF_VR_HAND_LEFT, 1, 0.1f));
        EXPECT_FALSE(host.Input->StopVRControllerVibration(MGDF_VR_HAND_LEFT));
        runtime.Push(XR_SESSION_STATE_FOCUSED);
        input.Connected[0] = false;
        runtime.SetInput(&input);
        ++phase;
      } else if (phase == 5) {
        EXPECT_FALSE(
            host.Input->GetVRController(MGDF_VR_HAND_LEFT, &controller));
        EXPECT_TRUE(
            host.Input->GetVRController(MGDF_VR_HAND_RIGHT, &controller));
        EXPECT_EQ(S_FALSE,
                  vr.GetControllerPose(MGDF_VR_HAND_LEFT,
                                       MGDF_VR_CONTROLLER_GRIP, &pose));
        input.FailSync = true;
        runtime.SetInput(&input);
        ++phase;
      } else if (phase == 6) {
        EXPECT_EQ(E_FAIL, vr.GetControllerPose(MGDF_VR_HAND_RIGHT,
                                               MGDF_VR_CONTROLLER_GRIP, &pose));
        EXPECT_FALSE(
            host.Input->GetVRController(MGDF_VR_HAND_RIGHT, &controller));
        EXPECT_EQ(0U, controller.Buttons);
        ++phase;
        return true;
      }
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
    EXPECT_EQ(1U, runtime.Stats().ActionSetsAttached);
    EXPECT_EQ(5U, runtime.Stats().SuggestedProfiles);
  }
  EXPECT_EQ(7U, phase);
  EXPECT_EQ(0U, runtime.Stats().Errors);
}

TEST_P(OpenXRIntegrationTests,
       GazeRequiresDeviceSupportAndAValidCurrentSample) {
  Runtime runtime;
  for (const bool tracking : {false, true}) {
    FakeXRConfig config{TestAdapter()};
    config.GazeExtension = true;
    config.EyeTracking = tracking;
    runtime.Configure(&config);
    FakeXRInput input;
    UINT32 phase = 0;
    {
      FrameLoop loop(GetParam(), true, true);
      ASSERT_TRUE(input::xinput::CreateInputManagerComponent(loop.Input));
      loop.Step = [&](auto &host) {
        auto &vr = host.Backend->VR();
        if (!vr.ViewCount()) return false;
        MGDFVRPose pose{};
        if (!tracking) {
          EXPECT_EQ(E_NOTIMPL, vr.GetGaze(&pose));
          EXPECT_FALSE(host.Input->GetVRGaze(&pose));
          ++phase;
          return true;
        }
        if (phase == 0) {
          EXPECT_EQ(S_OK, vr.GetGaze(&pose));
          EXPECT_TRUE(pose.OrientationValid);
          EXPECT_FLOAT_EQ(0.2f, pose.Position[2]);
          EXPECT_FLOAT_EQ(-0.6f, pose.Orientation[1]);
          EXPECT_TRUE(host.Input->GetVRGaze(&pose));
          EXPECT_FLOAT_EQ(0.2f, pose.Position[2]);
          input.GazeActive = false;
          runtime.SetInput(&input);
        } else if (phase == 1) {
          EXPECT_EQ(S_FALSE, vr.GetGaze(&pose));
          EXPECT_FALSE(host.Input->GetVRGaze(&pose));
          EXPECT_FALSE(pose.PositionValid);
          input.GazeActive = true;
          input.PoseFlags = XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
          runtime.SetInput(&input);
        } else if (phase == 2) {
          EXPECT_EQ(S_FALSE, vr.GetGaze(&pose));
          EXPECT_FALSE(host.Input->GetVRGaze(&pose));
          EXPECT_EQ(S_OK, vr.GetControllerPose(MGDF_VR_HAND_LEFT,
                                               MGDF_VR_CONTROLLER_GRIP, &pose));
          EXPECT_FALSE(pose.PositionValid);
          EXPECT_TRUE(pose.OrientationValid);
          input.FailSync = true;
          runtime.SetInput(&input);
        } else if (phase == 3) {
          EXPECT_EQ(E_FAIL, vr.GetGaze(&pose));
          EXPECT_FALSE(host.Input->GetVRGaze(&pose));
          ++phase;
          return true;
        }
        ++phase;
        return false;
      };
      loop.Execute();
      EXPECT_FALSE(loop.TimedOut);
      EXPECT_EQ(0U, loop.FatalErrors.load());
    }
    EXPECT_EQ(tracking ? 4U : 1U, phase);
    EXPECT_EQ(0U, runtime.Stats().Errors);
  }
}

TEST_P(OpenXRIntegrationTests, NativeCallsRejectWrongStateAndRuntimeFailure) {
  Runtime runtime;
  ASSERT_NE(nullptr, runtime.FailAcquire);
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0;
  {
    FrameLoop loop(GetParam(), true);
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      const MGDFVRPanel panel{80, 32, {0, 0, 2},          {0, 0, 0, 1},
                              1,  1,  MGDF_VR_SPACE_LOCAL};
      const auto check = [&](HRESULT expected, bool cached) {
        MGDFVREye view{};
        MGDFVRPose pose{};
        ComObject<IUnknown> target;
        EXPECT_EQ(expected, vr.GetView(0, &view, true));
        EXPECT_EQ(0U, view.Width);
        EXPECT_EQ(expected, vr.GetTarget(0, target));
        EXPECT_FALSE(target);
        EXPECT_EQ(expected, vr.GetDepthTarget(0, target));
        EXPECT_FALSE(target);
        EXPECT_EQ(expected, vr.SetVREyeDepthRange(0, 1, 10));
        EXPECT_EQ(expected, vr.GetPanelTarget(target));
        EXPECT_FALSE(target);
        EXPECT_EQ(expected,
                  vr.GetControllerPose(MGDF_VR_HAND_LEFT,
                                       MGDF_VR_CONTROLLER_GRIP, &pose));
        EXPECT_EQ(expected, vr.GetGaze(&pose));
        EXPECT_FALSE(pose.OrientationValid);
        UINT32 vertices = 9, indices = 9;
        UINT64 version = 9;
        EXPECT_EQ(cached ? S_OK : expected,
                  vr.GetVREyeMask(0, &vertices, nullptr, &indices, nullptr));
        EXPECT_EQ(cached ? S_OK : expected, vr.GetVREyeMaskVersion(&version));
        EXPECT_EQ(cached ? S_OK : expected, vr.SetVRPanel(&panel));
        if (!cached) {
          EXPECT_EQ(0U, vertices);
          EXPECT_EQ(0U, indices);
          EXPECT_EQ(0U, version);
        }
      };
      if (phase == 0) {
        check(E_ILLEGAL_METHOD_CALL, false);
        host.RequestVR(true);
        ++phase;
      } else if (phase == 1 && vr.ViewCount()) {
        std::thread wrongThread([&] { check(E_ILLEGAL_METHOD_CALL, false); });
        wrongThread.join();
        runtime.ShouldRender(false);
        ++phase;
      } else if (phase == 2 && !vr.ViewCount()) {
        // Gaze has no extension, but an eye-frame violation still takes
        // priority.
        check(E_ILLEGAL_METHOD_CALL, true);
        runtime.ShouldRender(true);
        runtime.FailAcquire();
        ++phase;
      } else if (phase == 3) {
        check(E_FAIL, false);
        ++phase;
        return true;
      }
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_EQ(4U, phase);
  EXPECT_EQ(0U, runtime.Stats().Errors);
}

TEST_P(OpenXRIntegrationTests,
       MaskRevisionAndActionsFollowRecreationBoundaries) {
  Runtime runtime;
  const FakeXRConfig config{TestAdapter()};
  runtime.Configure(&config);
  UINT32 phase = 0, changes = 0;
  UINT64 revision = 0;
  {
    FrameLoop loop(GetParam(), true, true);
    loop.Step = [&](auto &host) {
      auto &vr = host.Backend->VR();
      UINT64 current = 0;
      if (phase == 0 && vr.ViewCount()) {
        EXPECT_EQ(S_OK, vr.GetVREyeMaskVersion(&revision));
        changes = host.AfterChanges;
        ComObject<IMGDFPendingRenderSettingsChange> change;
        host.Settings->CreatePendingSettingsChange(change.Assign());
        change->SetVSync(FALSE);
        ++phase;
      } else if (phase == 1 && vr.ViewCount() && host.AfterChanges > changes) {
        EXPECT_EQ(S_OK, vr.GetVREyeMaskVersion(&current));
        EXPECT_NE(revision, current);
        revision = current;
        EXPECT_EQ(1U, runtime.Stats().SessionsCreated);
        EXPECT_EQ(1U, runtime.Stats().ActionSetsAttached);
        host.RequestVR(false);
        ++phase;
      } else if (phase == 2 && !host.Settings->GetVR()) {
        host.RequestVR(true);
        ++phase;
      } else if (phase == 3 && vr.ViewCount()) {
        EXPECT_EQ(S_OK, vr.GetVREyeMaskVersion(&current));
        EXPECT_NE(revision, current);
        EXPECT_EQ(2U, runtime.Stats().SessionsCreated);
        EXPECT_EQ(2U, runtime.Stats().ActionSetsAttached);
        ++phase;
        return true;
      }
      return false;
    };
    loop.Execute();
    EXPECT_FALSE(loop.TimedOut);
    EXPECT_EQ(0U, loop.FatalErrors.load());
  }
  EXPECT_EQ(4U, phase);
  EXPECT_EQ(0U, runtime.Stats().Errors);
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
