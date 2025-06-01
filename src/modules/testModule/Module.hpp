#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <functional>

#include "BufferedGameState.hpp"
#include "TextManager.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

struct TestResults {
  uint32_t Passed;
  uint32_t Failed;
};

class TestState {
 public:
  TestState() {}
  TestState(const TestState &state);
  TestState(const TestState &startState, const TestState &endState,
            double alpha);
  virtual ~TestState() {}
  TextManagerState Text;
};

class TestModule {
 public:
  enum class TestStep { FAILED, PASSED, CONT, NEXT };

  TestModule() : _testIndex(0) {}
  virtual ~TestModule(void) {}
  bool Update(IMGDFSimHost *host, std::shared_ptr<TestState> &state,
              TestResults &results);
  TestModule &Step(std::function<TestStep(std::shared_ptr<TestState> &)> step);
  TestModule &StepOnce(std::function<void(std::shared_ptr<TestState> &)> step);

 protected:
  virtual void Setup(IMGDFSimHost *host) = 0;

 private:
  std::vector<std::function<TestStep(std::shared_ptr<TestState> &)>> _steps;
  int _testIndex;
};

class Module : public ComBase<IMGDFModule> {
 public:
  virtual ~Module(void);
  Module();

  BOOL __stdcall STNew(IMGDFSimHost *simHost) final;
  BOOL __stdcall STUpdate(IMGDFSimHost *simHost, double elapsedTime) final;
  void __stdcall STShutDown(IMGDFSimHost *simHost) final;
  BOOL __stdcall RTBeforeFirstDraw(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTDraw(IMGDFRenderHost *renderHost, double alpha) final;
  BOOL __stdcall RTBeforeBackBufferChange(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTBackBufferChange(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTBeforeDeviceReset(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTDeviceReset(IMGDFRenderHost *renderHost) final;
  void __stdcall RTShutDown(IMGDFRenderHost *renderHost) final;
  void __stdcall Panic() final;

 private:
  TestResults _results;
  bool _finalResult;
  std::list<std::unique_ptr<TestModule>> _testModules;
  std::list<std::unique_ptr<TestModule>>::iterator _currentModule;
  BufferedGameState<TestState> _stateBuffer;
  std::unique_ptr<TextManager> _textManager;
  ComObject<IMGDFPerformanceCounter> _textManagerCounter;
  ComObject<IMGDFPerformanceCounter> _testModuleCounter;
  ComObject<IMGDFInputManager> _input;
};

}  // namespace Test
}  // namespace MGDF
