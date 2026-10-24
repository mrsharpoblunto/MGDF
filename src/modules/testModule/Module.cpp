#include "StdAfx.h"

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
    : _finalResult(false),
      _results({0, 0}),
      _textManagerCounter(nullptr),
      _testModuleCounter(nullptr),
      _textManager(nullptr),
      _draggingScrollbar(false),
      _wheelAccumulator(0) {
  _stateBuffer.Pending()->Text.AddLine("MGDF functional test suite started");
}

BOOL Module::STNew(IMGDFSimHost *host) {
  auto commonHost = MakeComFromPtr<IMGDFSimHost>(host).As<IMGDFCommonHost>();
  if (!commonHost) return false;
  const auto api = commonHost->GetGraphicsAPI();
  auto d3d11Host = commonHost.As<IMGDFD3D11Host>();
  auto d3d12Host = commonHost.As<IMGDFD3D12Host>();
  if ((api == MGDF_GRAPHICS_API_D3D11 && (!d3d11Host || d3d12Host)) ||
      (api == MGDF_GRAPHICS_API_D3D12 && (!d3d12Host || d3d11Host)))
    return false;
  auto simHost = commonHost.As<IMGDFSimHost>();
  if (!simHost) return false;
  ComObject<IMGDFGame> game;
  simHost->GetGame(game.Assign());
  if (!game) return false;
  if (d3d11Host) {
    ComObject<ID3D11Device> device;
    d3d11Host->GetD3D11Device(device.Assign());
    if (!device) return false;
  } else {
    ComObject<ID3D12Device10> device;
    d3d12Host->GetD3D12Device(device.Assign());
    if (!device) return false;
  }
  // mirror all test output into the MGDF log so that test results and
  // failure details are available in the core log after a run
  TextManagerState::LogSink = [host](const std::string &line) {
    host->Log("TestModule", line.c_str(), MGDF_LOG_LOW);
  };
  host->GetRenderSettings(_renderSettings.Assign());
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

    // mousewheel scrolling of the test output
    const UINT32 screenX = _renderSettings->GetScreenX();
    const UINT32 screenY = _renderSettings->GetScreenY();
    _wheelAccumulator += _input->GetMouseDZ();
    const INT32 wheelLines = (_wheelAccumulator * 3) / WHEEL_DELTA;
    if (wheelLines) {
      _wheelAccumulator -= (wheelLines * WHEEL_DELTA) / 3;
      state->Text.Scroll(wheelLines, screenY);
    }

    // dragging the scrollbar on the right of the screen
    if (_input->IsButtonDown(MGDF_MOUSE_LEFT)) {
      // use a slightly wider hit area than the rendered scrollbar to make
      // it easier to grab
      const INT32 hitArea = static_cast<INT32>(
          static_cast<float>(screenX) - 2 * TextManagerState::SCROLLBAR_WIDTH);
      if (_draggingScrollbar || _input->GetMouseX() >= hitArea) {
        _draggingScrollbar = true;
        state->Text.SetScroll(static_cast<float>(_input->GetMouseY()), screenY);
      }
    } else {
      _draggingScrollbar = false;
    }

    if (_input->IsKeyPress(VK_ESCAPE)) {
      host->QueueShutDown();
    }
    if (_input->IsKeyDown(VK_MENU) && _input->IsKeyPress(VK_F12)) {
      state->Text.ToggleOverlay();
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
  if (host->GetGraphicsAPI() == MGDF_GRAPHICS_API_D3D12) return InitD3D12(host);
  _textManager = std::make_unique<TextManager>(host);
  ComObject<ID3D11Device> device;
  auto d3d11Host = MakeComFromPtr<IMGDFRenderHost>(host).As<IMGDFD3D11Host>();
  auto d3d11RenderHost =
      MakeComFromPtr<IMGDFRenderHost>(host).As<IMGDFD3D11RenderHost>();
  if (!d3d11Host || !d3d11RenderHost) return false;
  d3d11Host->GetD3D11Device(device.Assign());
  MGDFBackBufferInfo info{};
  host->GetBackBufferInfo(&info);
  D3D11_TEXTURE2D_DESC desc{};
  d3d11RenderHost->GetBackBufferDescription(&desc, nullptr);
  if (info.Width != desc.Width || info.Height != desc.Height ||
      info.Format != desc.Format || info.SampleCount != desc.SampleDesc.Count ||
      !info.Width || !info.Height)
    return false;
  ComObject<ID3D11DeviceContext> context;
  device->GetImmediateContext(context.Assign());
  ComObject<IMGDFMetric> gauge;
  host->CreateGaugeMetric("text_rendering", "Text rendering time",
                          gauge.Assign());
  d3d11RenderHost->CreateGPUCounter(gauge, context,
                                    _textManagerCounter.Assign());
  return true;
}

BOOL Module::RTDraw(IMGDFRenderHost *host, double alpha) {
  std::ignore = host;
  if (_awaitingPresent) return false;
  _awaitingPresent = true;
  if (host->GetGraphicsAPI() == MGDF_GRAPHICS_API_D3D12) {
    if (_presentedFrames && _presentedFrames % 240 == 0) return false;
    if (!DrawD3D12(host, alpha))
      host->FatalError("TestModule", "D3D12 frame validation failed");
    return true;
  }
  std::shared_ptr<TestState> state = _stateBuffer.Interpolate(alpha);
  if (state) {
    ComObject<IMGDFPerformanceCounterScope> counter;
    if (_textManagerCounter)
      _textManagerCounter->Begin(nullptr, counter.Assign());
    _textManager->SetState(state->Text);
    _textManager->DrawText();
  }
  return true;
}

BOOL Module::RTBackBufferChange(IMGDFRenderHost *host) {
  std::ignore = host;
  if (_textManager) _textManager->BackBufferChange();
  return true;
}

void Module::RTAfterPresent(IMGDFRenderHost *host) {
  if (!_awaitingPresent) {
    host->FatalError("TestModule", "RTAfterPresent called without RTDraw");
    return;
  }
  _awaitingPresent = false;
  if (_presentedFrames == 120) {
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
    host->Log(
        "TestModule",
        host->GetGraphicsAPI() == MGDF_GRAPHICS_API_D3D12
            ? "D3D12 host interfaces, frame slots and RTAfterPresent verified"
            : "D3D11 host interfaces and RTAfterPresent verified",
        MGDF_LOG_LOW);
  }
}

BOOL Module::RTBeforeBackBufferChange(IMGDFRenderHost *host) {
  std::ignore = host;
  if (_textManager) _textManager->BeforeBackBufferChange();
  return true;
}

BOOL Module::RTBeforeDeviceReset(IMGDFRenderHost *host) {
  std::ignore = host;
  if (_textManager) _textManager->BeforeDeviceReset();
  _textManagerCounter.Clear();
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
  _textManagerCounter.Clear();
  _textManager.reset();
  ReleaseD3D12();
}

bool Module::InitD3D12(IMGDFRenderHost *host) {
  auto common = MakeComFromPtr<IMGDFRenderHost>(host);
  auto deviceHost = common.As<IMGDFD3D12Host>();
  auto renderHost = common.As<IMGDFD3D12RenderHost>();
  if (!deviceHost || !renderHost || common.As<IMGDFD3D11RenderHost>())
    return false;
  deviceHost->GetD3D12Device(_d3d12Device.Assign());
  deviceHost->GetDirectQueue(_directQueue.Assign());
  renderHost->GetFrameFence(_frameFence.Assign());
  ComObject<ID3D12CommandQueue> compute, copy;
  deviceHost->GetComputeQueue(compute.Assign());
  deviceHost->GetCopyQueue(copy.Assign());
  if (!_d3d12Device || !_directQueue || !_frameFence || !compute || !copy)
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
        renderHost->CreateGPUCounter(metric, list, rejected.Assign()) !=
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
        FAILED(renderHost->CreateGPUCounter(metric, _lists[i].Get(),
                                            _gpuCounters[i].Assign())))
      return false;
  }
  return true;
}

bool Module::DrawD3D12(IMGDFRenderHost *host, double elapsed) {
  auto renderHost =
      MakeComFromPtr<IMGDFRenderHost>(host).As<IMGDFD3D12RenderHost>();
  MGDFFrameInfo frame{};
  renderHost->GetCurrentFrame(&frame);
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
    list->ClearRenderTargetView(frame.BackBufferRTV, color, 0, nullptr);
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
  _frameFence.Clear();
  _directQueue.Clear();
  _d3d12Device.Clear();
  _slotFences = {};
}

void Module::Panic() {}

}  // namespace Test
}  // namespace MGDF