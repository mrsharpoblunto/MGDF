#include "StdAfx.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "DisplayTests.hpp"
#include "InputTests.hpp"
#include "LoadSaveTests.hpp"
#include "NetworkTests.hpp"
#include "SoundTests.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

namespace {
std::array<float, 4> EyeColor(const MGDFVREye &view, UINT32 index) {
  const auto &q = view.Orientation;
  const float yaw = std::atan2(2 * (q[3] * q[1] + q[0] * q[2]),
                               1 - 2 * (q[0] * q[0] + q[1] * q[1]));
  const float pitch =
      std::asin(std::clamp(2 * (q[3] * q[0] - q[1] * q[2]), -1.0f, 1.0f));
  const float brightness = 0.5f + 0.25f * std::sin(pitch);
  return {brightness * (0.5f + 0.4f * std::sin(yaw)),
          brightness * (index ? 0.35f : 0.85f),
          brightness * (0.5f + 0.4f * std::cos(yaw)), 1.0f};
}
}  // namespace

TestState::TestState(const TestState &state) : Text(state.Text) {}

TestState::TestState(const TestState &startState, const TestState &endState,
                     double alpha)
    : Text(startState.Text, endState.Text, alpha) {}

bool TestModule::Update(IMGDFSimHost *host, std::shared_ptr<TestState> &state,
                        TestResults &results) {
  ComObject<IMGDFInputManager> input;
  host->GetInput(input.Assign());
  if (input->IsKeyPress(VK_ESCAPE)) {
    host->ShutDown();
  }

  if (!_steps.size()) {
    Setup(host);
  } else if (_testIndex < _steps.size()) {
    auto result = _steps[_testIndex](state);
    if (result == TestStep::PASSED) {
      state->Text.SetStatus(TextColor::GREEN, "[Test Passed]");
      ++_testIndex;
      ++results.Passed;
    } else if (result == TestStep::FAILED) {
      state->Text.SetStatus(TextColor::RED, "[Test Failed]");
      if (!state->LastError.empty()) {
        state->Text.AddLine("  ERROR: " + state->LastError);
        state->LastError.clear();
      }
      _testIndex = static_cast<int>(_steps.size());
      ++results.Failed;
      return true;
    } else if (result == TestStep::NEXT) {
      ++_testIndex;
    }
  }
  return _testIndex == _steps.size();
}

TestModule &TestModule::Step(
    std::function<TestStep(std::shared_ptr<TestState> &)> step) {
  _steps.push_back(step);
  return *this;
}

TestModule &TestModule::StepOnce(
    std::function<void(std::shared_ptr<TestState> &)> step) {
  _steps.push_back([step](auto state) {
    step(state);
    return TestStep::NEXT;
  });
  return *this;
}

Module::~Module(void) {}

Module::Module()
    : _finalResult(false), _results({0, 0}), _testModuleCounter(nullptr) {
  _stateBuffer.Pending()->Text.AddLine("MGDF functional test suite started");
}

BOOL Module::STNew(IMGDFSimHost *host) {
  auto commonHost = MakeComFromPtr<IMGDFSimHost>(host).As<IMGDFCommonHost>();
  if (!commonHost) return false;
  auto simHost = commonHost.As<IMGDFSimHost>();
  if (!simHost) return false;
  ComObject<IMGDFGame> game;
  simHost->GetGame(game.Assign());
  if (!game) return false;
  ComObject<ID3D12Device10> device;
  commonHost->GetD3D12Device(device.Assign());
  if (!device) return false;

  // mirror all test output into the MGDF log so that test results and
  // failure details are available in the core log after a run
  TextManagerState::LogSink = [host](const std::string &line) {
    host->Log("TestModule", line.c_str(), MGDF_LOG_LOW);
  };
  host->GetRenderSettings(_renderSettings.Assign());
  _vrTest = _renderSettings->GetVRSupport() != FALSE;
  if (_vrTest) {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    _renderSettings->CreatePendingSettingsChange(change.Assign());
    change->SetVR(TRUE);
    host->Log("TestModule", "VR display test: F9 toggles VR; ESC exits",
              MGDF_LOG_LOW);
    _currentModule = _testModules.end();
    return true;
  }
  _testModules.push_back(std::make_unique<DisplayTests>());
  _testModules.push_back(std::make_unique<LoadSaveTests>());
  _testModules.push_back(std::make_unique<NetworkTests>());
  _testModules.push_back(std::make_unique<SoundTests>());
  _testModules.push_back(std::make_unique<InputTests>());
  _currentModule = _testModules.begin();

  return true;
}

BOOL Module::STUpdate(IMGDFSimHost *host, double elapsedTime) {
  std::ignore = elapsedTime;
  if (!_testModuleCounter) {
    ComObject<IMGDFMetric> gauge;
    host->CreateGaugeMetric("test_module", "a test counter", gauge.Assign());
    host->CreateCPUCounter(gauge, _testModuleCounter.Assign());
  }
  _ASSERTE(_testModuleCounter);
  if (!_input) {
    host->GetInput(_input.Assign());
    _vrControllers =
        ComArray<IMGDFVRController>(_input->GetVRControllerCount());
    _input->GetVRControllers(_vrControllers.Data());
  }

  if (_vrTest) {
    for (UINT32 i = 0; i < 2; ++i) {
      const auto controller = _vrControllers[i];
      const bool connected = controller->IsConnected() != FALSE;
      if (connected && !_profileLogged[i]) {
        const char *profile = "Other";
        switch (controller->GetProfile()) {
          case MGDF_VR_PROFILE_TOUCH:
            profile = "Touch";
            break;
          case MGDF_VR_PROFILE_INDEX:
            profile = "Index";
            break;
          case MGDF_VR_PROFILE_VIVE:
            profile = "Vive";
            break;
          case MGDF_VR_PROFILE_WMR:
            profile = "WMR";
            break;
          case MGDF_VR_PROFILE_SIMPLE:
            profile = "Simple";
            break;
          default:
            break;
        }
        const auto message =
            std::string(i ? "Right VR controller: " : "Left VR controller: ") +
            profile;
        host->Log("TestModule", message.c_str(), MGDF_LOG_LOW);
        _profileLogged[i] = true;
      }
      const bool pressed = connected && controller->GetTrigger() > 0.7f;
      if (pressed && !_triggerPressed[i]) _vibrationSeconds[i] = 0.2;
      controller->SetVibration(_vibrationSeconds[i] > 0 ? 0.6f : 0);
      _vibrationSeconds[i] -= elapsedTime;
      _triggerPressed[i] = pressed;
    }
  }

  if (_input->IsKeyPress(VK_F9)) {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    _renderSettings->CreatePendingSettingsChange(change.Assign());
    change->SetVR(!_renderSettings->GetVR());
  }
  if (_input->IsKeyPress(VK_F11)) {
    MGDFFullScreenDesc desc{};
    _renderSettings->GetFullscreen(&desc);
    desc.FullScreen = !desc.FullScreen;
    desc.ExclusiveMode = FALSE;
    ComObject<IMGDFPendingRenderSettingsChange> change;
    _renderSettings->CreatePendingSettingsChange(change.Assign());
    change->SetFullscreen(&desc);
    host->Log(
        "TestModule",
        desc.FullScreen ? "Smoke: borderless fullscreen" : "Smoke: windowed",
        MGDF_LOG_LOW);
  }
  if (_input->IsKeyPress(VK_F8)) {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    _renderSettings->CreatePendingSettingsChange(change.Assign());
    change->SetVSync(!_renderSettings->GetVSync());
    host->Log("TestModule", "Smoke: vsync toggled", MGDF_LOG_LOW);
  }
  {
    ComObject<IMGDFPerformanceCounterScope> counterScope;
    _testModuleCounter->Begin(nullptr, counterScope.Assign());
    auto state = _stateBuffer.Pending();
    if (_currentModule != _testModules.end()) {
      const auto moduleComplete =
          (*_currentModule)->Update(host, state, _results);

      if (moduleComplete) {
        ++_currentModule;
      }
    } else if (!_finalResult) {
      _finalResult = true;
      std::ostringstream oss;
      oss << _results.Passed << "/" << (_results.Passed + _results.Failed)
          << " tests passed. Press the [ESC] key to exit (then make sure there "
             "were no memory leaks)";
      state->Text.AddLine("");
      state->Text.AddLine(oss.str());
      state->Text.AddLine("");
    }

    if (_input->IsKeyPress(VK_ESCAPE)) {
      host->QueueShutDown();
    }
  }

  _stateBuffer.Flip();
  return true;
}

void Module::STShutDown(IMGDFSimHost *host) {
  TextManagerState::LogSink = nullptr;
  host->ShutDown();
}

BOOL Module::RTBeforeFirstDraw(IMGDFRenderHost *host) {
  return InitD3D12(host);
}

BOOL Module::RTDraw(IMGDFRenderHost *host, double alpha) {
  if (_awaitingPresent) return false;
  _awaitingPresent = true;
  if (!_vrTest && _presentedFrames && _presentedFrames % 240 == 0) return false;
  if (!DrawD3D12(host, alpha))
    host->FatalError("TestModule", "D3D12 frame validation failed");
  return true;
}

BOOL Module::RTBackBufferChange(IMGDFRenderHost *host) {
  MGDFVRFrame frame{};
  if (_vrTest && SUCCEEDED(host->GetVRFrame(&frame)) && frame.Eyes[0].Width) {
    const MGDFVRPanel panel{512,  256,   {0, 0, 1.5f},      {0, 0, 0, 1},
                            0.7f, 0.35f, MGDF_VR_SPACE_VIEW};
    host->SetVRPanel(&panel);
  }
  return true;
}

void Module::RTAfterPresent(IMGDFRenderHost *host) {
  if (!_awaitingPresent) {
    host->FatalError("TestModule", "RTAfterPresent called without RTDraw");
    return;
  }
  _awaitingPresent = false;
  if (!_vrTest && _presentedFrames == 120) {
    ComObject<IMGDFDebug> debug;
    host->GetDebug(debug.Assign());
    ComObject<IMGDFDebugOverlaySnapshot> snapshot;
    debug->GetOverlaySnapshot(snapshot.Assign());
    const auto data = snapshot->GetData();
    bool found = false;
    for (UINT64 i = 0; i < data->CounterCount; ++i) {
      if (data->Counters[i].GPU && data->Counters[i].Timing.SampleCount) {
        found = true;
        std::ostringstream message;
        message << "GPU overlay counter verified: " << data->Counters[i].Name
                << ", seconds " << data->Counters[i].Timing.Average;
        host->Log("TestModule", message.str().c_str(), MGDF_LOG_LOW);
      }
    }
    if (!found)
      host->FatalError("TestModule",
                       "GPU counter missing from overlay snapshot");
  }
  if (++_presentedFrames == 1) {
    host->Log("TestModule",
              "D3D12 host interfaces, frame slots and RTAfterPresent verified",
              MGDF_LOG_LOW);
  }
}

BOOL Module::RTBeforeBackBufferChange(IMGDFRenderHost *host) {
  std::ignore = host;
  return true;
}

BOOL Module::RTBeforeDeviceReset(IMGDFRenderHost *host) {
  std::ignore = host;
  ReleaseD3D12();
  host->QueueDeviceReset();
  return true;
}

BOOL Module::RTDeviceReset(IMGDFRenderHost *host) {
  _awaitingPresent = false;
  return RTBeforeFirstDraw(host);
}

void Module::RTShutDown(IMGDFRenderHost *host) {
  std::ignore = host;
  ReleaseD3D12();
}

bool Module::InitD3D12(IMGDFRenderHost *host) {
  host->GetD3D12Device(_d3d12Device.Assign());
  host->GetDirectQueue(_directQueue.Assign());
  host->GetFrameFence(_frameFence.Assign());
  ComObject<ID3D12CommandQueue> compute, copy;
  host->GetComputeQueue(compute.Assign());
  host->GetCopyQueue(copy.Assign());
  if (!_d3d12Device || !_directQueue || !_frameFence || !compute || !copy)
    return false;
  const D3D12_DESCRIPTOR_HEAP_DESC heap{.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                        .NumDescriptors = 6};
  if (FAILED(_d3d12Device->CreateDescriptorHeap(
          &heap, IID_PPV_ARGS(_eyeRTVs.Assign()))))
    return false;
  ComObject<IMGDFMetric> metric;
  host->CreateGaugeMetric("d3d12_clear", "D3D12 backbuffer clear time",
                          metric.Assign());
  for (auto type :
       {D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_TYPE_COMPUTE}) {
    ComObject<ID3D12CommandAllocator> allocator;
    ComObject<ID3D12GraphicsCommandList> list;
    ComObject<IMGDFPerformanceCounter> rejected;
    if (FAILED(_d3d12Device->CreateCommandAllocator(
            type, IID_PPV_ARGS(allocator.Assign()))) ||
        FAILED(_d3d12Device->CreateCommandList(0, type, allocator, nullptr,
                                               IID_PPV_ARGS(list.Assign()))) ||
        host->CreateGPUCounter(metric, list, rejected.Assign()) !=
            E_INVALIDARG ||
        rejected || FAILED(list->Close()))
      return false;
  }
  host->Log("TestModule",
            "Compute/copy GPU counters rejected with E_INVALIDARG",
            MGDF_LOG_LOW);
  for (UINT i = 0; i < _lists.size(); ++i) {
    if (FAILED(_d3d12Device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(_allocators[i].Assign()))) ||
        FAILED(_lists[i].Create(_d3d12Device, _allocators[i])) ||
        FAILED(_lists[i].Close()) ||
        FAILED(host->CreateGPUCounter(metric, _lists[i].Get(),
                                      _gpuCounters[i].Assign())))
      return false;
  }
  return true;
}

bool Module::DrawD3D12(IMGDFRenderHost *host, double elapsed) {
  MGDFFrameInfo frame{};
  host->GetCurrentFrame(&frame);
  const UINT slot = frame.FrameSlot;
  if (frame.FramesInFlight != _lists.size() || slot >= _lists.size() ||
      slot != frame.FrameOrdinal % frame.FramesInFlight ||
      frame.CompletedOrdinal < _slotFences[slot] || !frame.BackBuffer ||
      frame.BackBufferIndex >= 3)
    return false;
  _slotFences[slot] = frame.FrameOrdinal;
  if (FAILED(_allocators[slot]->Reset()) ||
      FAILED(_lists[slot].Reset(_allocators[slot])))
    return false;
  auto list = _lists[slot].Get();
  _renderTime += elapsed;
  {
    ComObject<IMGDFPerformanceCounterScope> scope;
    if (FAILED(_gpuCounters[slot]->Begin(nullptr, scope.Assign())))
      return false;
    D3D12_RESOURCE_BARRIER barrier{
        .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
        .Transition = {.pResource = frame.BackBuffer,
                       .Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
                       .StateBefore = D3D12_RESOURCE_STATE_PRESENT,
                       .StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET}};
    list->ResourceBarrier(1, &barrier);
    const float color[4] = {
        0.25f + 0.2f * static_cast<float>(std::sin(_renderTime)),
        0.25f + 0.2f * static_cast<float>(std::sin(_renderTime + 2)),
        0.25f + 0.2f * static_cast<float>(std::sin(_renderTime + 4)), 1};
    MGDFVRFrame vr{};
    host->GetVRFrame(&vr);
    for (UINT32 eye = 0; eye < vr.EyeCount; ++eye) {
      ComObject<ID3D12Resource> target;
      const auto &view = vr.Eyes[eye];
      if (host->GetVREyeTarget(eye, target.Assign()) != S_OK) return false;
      auto rtv = _eyeRTVs->GetCPUDescriptorHandleForHeapStart();
      rtv.ptr += (static_cast<SIZE_T>(slot) * 3 + eye) *
                 _d3d12Device->GetDescriptorHandleIncrementSize(
                     D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
      const D3D12_RENDER_TARGET_VIEW_DESC desc{
          .Format = view.Format,
          .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D};
      _d3d12Device->CreateRenderTargetView(target, &desc, rtv);
      const auto eyeColor = EyeColor(view, eye);
      list->ClearRenderTargetView(rtv, eyeColor.data(), 0, nullptr);
      if (!eye) _leftEyeColor = eyeColor;
    }
    ComObject<ID3D12Resource> panel;
    if (host->GetVRPanelTarget(panel.Assign()) == S_OK) {
      const auto &view = vr.Eyes[0];
      auto rtv = _eyeRTVs->GetCPUDescriptorHandleForHeapStart();
      rtv.ptr += (static_cast<SIZE_T>(slot) * 3 + 2) *
                 _d3d12Device->GetDescriptorHandleIncrementSize(
                     D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
      const D3D12_RENDER_TARGET_VIEW_DESC desc{
          .Format = view.Format,
          .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D};
      _d3d12Device->CreateRenderTargetView(panel, &desc, rtv);
      const float panelColor[4]{0.08f, 0.4f, 0.16f, 0.8f};
      list->ClearRenderTargetView(rtv, panelColor, 0, nullptr);
    }
    list->ClearRenderTargetView(frame.BackBufferRTV,
                                _vrTest ? _leftEyeColor.data() : color, 0,
                                nullptr);
    std::swap(barrier.Transition.StateBefore, barrier.Transition.StateAfter);
    list->ResourceBarrier(1, &barrier);
  }
  ComObject<IMGDFPerformanceCounterScope> lateScope;
  if (_presentedFrames == 1 &&
      GetEnvironmentVariableA("MGDF_TEST_LATE_GPU_SCOPE", nullptr, 0)) {
    if (FAILED(_gpuCounters[slot]->Begin(nullptr, lateScope.Assign())))
      return false;
  }
  if (FAILED(_lists[slot].Close())) return false;
  lateScope.Clear();
  ID3D12CommandList *lists[] = {list};
  _directQueue->ExecuteCommandLists(1, lists);
  return true;
}

void Module::ReleaseD3D12() {
  for (auto &counter : _gpuCounters) counter.Clear();
  for (auto &list : _lists) list.Clear();
  for (auto &allocator : _allocators) allocator.Clear();
  _eyeRTVs.Clear();
  _frameFence.Clear();
  _directQueue.Clear();
  _d3d12Device.Clear();
  _slotFences = {};
}

void Module::Panic() {}

}  // namespace Test
}  // namespace MGDF