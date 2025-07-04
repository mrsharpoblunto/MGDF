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
      _textManager(nullptr) {
  _stateBuffer.Pending()->Text.AddLine("MGDF functional test suite started");
}

BOOL Module::STNew(IMGDFSimHost* host) {
  std::ignore = host;
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
    if (_input->IsKeyPress(VK_ESCAPE)) {
      host->QueueShutDown();
    }
  }

  _stateBuffer.Flip();
  return true;
}

void Module::STShutDown(IMGDFSimHost* host) { host->ShutDown(); }

BOOL Module::RTBeforeFirstDraw(IMGDFRenderHost* host) {
  _textManager = std::make_unique<TextManager>(host);
  ComObject<ID3D11Device> device;
  host->GetD3DDevice(device.Assign());
  ComObject<ID3D11DeviceContext> context;
  device->GetImmediateContext(context.Assign());
  ComObject<IMGDFMetric> gauge;
  host->CreateGaugeMetric("text_rendering", "Text rendering time",
                          gauge.Assign());
  host->CreateGPUCounter(gauge, context, _textManagerCounter.Assign());
  return true;
}

BOOL Module::RTDraw(IMGDFRenderHost* host, double alpha) {
  std::ignore = host;
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