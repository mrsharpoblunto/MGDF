#include "StdAfx.h"

#include <fstream>

#include "Test1.hpp"
#include "Test2.hpp"
#include "Test3.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

TestModule* TestModule::Update(IMGDFSimHost* host, TextManagerState* state) {
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
      state->SetStatus(TextColor::GREEN, "[Test Passed]");
      ++_testIndex;
      return _testIndex == _steps.size() ? NextTestModule() : nullptr;
    } else if (result == TestStep::FAILED) {
      state->SetStatus(TextColor::RED, "[Test Failed]");
      auto next = NextTestModule();
      if (!next) {
        _testIndex = static_cast<int>(_steps.size());
      }
      return next;
    } else if (result == TestStep::NEXT) {
      ++_testIndex;
    }
  }
  return nullptr;
}

TestModule& TestModule::Step(std::function<TestStep(TextManagerState*)> step) {
  _steps.push_back(step);
  return *this;
}

TestModule& TestModule::StepOnce(std::function<void(TextManagerState*)> step) {
  _steps.push_back([step](auto state) {
    step(state);
    return TestStep::NEXT;
  });
  return *this;
}

Module::~Module(void) {}

Module::Module()
    : _textManagerCounter(nullptr),
      _testModuleCounter(nullptr),
      _textManager(nullptr),
      _testModule(nullptr) {
  _stateBuffer.Pending()->AddLine("MGDF functional test suite started");
}

BOOL Module::STNew(IMGDFSimHost* host, const wchar_t* workingFolder) {
  (void)host;
  _workingFolder = workingFolder;
  _testModule = std::make_unique<Test1>();

  return true;
}

BOOL Module::STUpdate(IMGDFSimHost* host, double elapsedTime) {
  (void)elapsedTime;
  if (!_testModuleCounter) {
    host->CreateCPUCounter("Test Module", _testModuleCounter.Assign());
  }

  if (_testModuleCounter) _testModuleCounter->Begin();
  auto next = std::unique_ptr<TestModule>(
      _testModule->Update(host, _stateBuffer.Pending()));

  if (next != nullptr) {
    _testModule.swap(next);
  }
  if (_testModuleCounter) _testModuleCounter->End();

  _stateBuffer.Flip();
  return true;
}

void Module::STShutDown(IMGDFSimHost* host) { host->ShutDown(); }

BOOL Module::RTBeforeFirstDraw(IMGDFRenderHost* host) {
  _textManager = std::make_unique<TextManager>(host);
  host->CreateGPUCounter("Text Rendering", _textManagerCounter.Assign());
  return true;
}

BOOL Module::RTDraw(IMGDFRenderHost* host, double alpha) {
  (void)host;
  std::shared_ptr<TextManagerState> state = _stateBuffer.Interpolate(alpha);
  if (state) {
    if (_textManagerCounter) _textManagerCounter->Begin();
    _textManager->SetState(state);
    _textManager->DrawText();
    if (_textManagerCounter) _textManagerCounter->End();
  }
  return true;
}

BOOL Module::RTBackBufferChange(IMGDFRenderHost* host) {
  (void)host;
  _textManager->BackBufferChange();
  return true;
}

BOOL Module::RTBeforeBackBufferChange(IMGDFRenderHost* host) {
  (void)host;
  _textManager->BeforeBackBufferChange();
  return true;
}

BOOL Module::RTBeforeDeviceReset(IMGDFRenderHost* host) {
  (void)host;
  _textManager->BeforeDeviceReset();
  return true;
}

BOOL Module::RTDeviceReset(IMGDFRenderHost* host) {
  (void)host;
  return true;
}

void Module::Panic() {}

}  // namespace Test
}  // namespace MGDF