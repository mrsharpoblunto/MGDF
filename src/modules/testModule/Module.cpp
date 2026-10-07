#include "StdAfx.h"

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

TestState::TestState(const TestState& state) : Text(state.Text) {}

TestState::TestState(const TestState& startState, const TestState& endState,
                     double alpha)
    : Text(startState.Text, endState.Text, alpha) {}

bool TestModule::Update(IMGDFSimHost* host, std::shared_ptr<TestState>& state,
                        TestResults& results) {
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

TestModule& TestModule::Step(
    std::function<TestStep(std::shared_ptr<TestState>&)> step) {
  _steps.push_back(step);
  return *this;
}

TestModule& TestModule::StepOnce(
    std::function<void(std::shared_ptr<TestState>&)> step) {
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

BOOL Module::STNew(IMGDFSimHost* host) {
  auto commonHost = MakeComFromPtr<IMGDFSimHost>(host).As<IMGDFCommonHost>();
  auto d3d11Host = MakeComFromPtr<IMGDFSimHost>(host).As<IMGDFD3D11Host>();
  if (!commonHost || !d3d11Host ||
      commonHost->GetGraphicsAPI() != MGDF_GRAPHICS_API_D3D11)
    return false;
  auto simHost = commonHost.As<IMGDFSimHost>();
  if (!simHost) return false;
  ComObject<IMGDFGame> game;
  simHost->GetGame(game.Assign());
  ComObject<ID3D11Device> device;
  d3d11Host->GetD3D11Device(device.Assign());
  if (!game || !device) return false;
  // mirror all test output into the MGDF log so that test results and
  // failure details are available in the core log after a run
  TextManagerState::LogSink = [host](const std::string& line) {
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

BOOL Module::STUpdate(IMGDFSimHost* host, double elapsedTime) {
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
        state->Text.SetScroll(static_cast<float>(_input->GetMouseY()),
                              screenY);
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

void Module::STShutDown(IMGDFSimHost* host) {
  TextManagerState::LogSink = nullptr;
  host->ShutDown();
}

BOOL Module::RTBeforeFirstDraw(IMGDFRenderHost* host) {
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

BOOL Module::RTDraw(IMGDFRenderHost* host, double alpha) {
  std::ignore = host;
  if (_awaitingPresent) return false;
  _awaitingPresent = true;
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

BOOL Module::RTBackBufferChange(IMGDFRenderHost* host) {
  std::ignore = host;
  _textManager->BackBufferChange();
  return true;
}

void Module::RTAfterPresent(IMGDFRenderHost *host) {
  if (!_awaitingPresent) {
    host->FatalError("TestModule", "RTAfterPresent called without RTDraw");
    return;
  }
  _awaitingPresent = false;
  if (++_presentedFrames == 1) {
    host->Log("TestModule", "D3D11 host interfaces and RTAfterPresent verified",
              MGDF_LOG_LOW);
  }
}

BOOL Module::RTBeforeBackBufferChange(IMGDFRenderHost* host) {
  std::ignore = host;
  _textManager->BeforeBackBufferChange();
  return true;
}

BOOL Module::RTBeforeDeviceReset(IMGDFRenderHost* host) {
  std::ignore = host;
  _textManager->BeforeDeviceReset();
  return true;
}

BOOL Module::RTDeviceReset(IMGDFRenderHost* host) {
  std::ignore = host;
  return true;
}

void Module::RTShutDown(IMGDFRenderHost* host) {
  std::ignore = host;
  _textManagerCounter.Clear();
  _textManager.reset();
}

void Module::Panic() {}

}  // namespace Test
}  // namespace MGDF