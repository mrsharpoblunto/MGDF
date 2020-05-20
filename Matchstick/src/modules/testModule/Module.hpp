#pragma once

#include <time.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <functional>

#include "BufferedGameState.hpp"
#include "TextManager.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

class TestModule {
 public:
  enum class TestStep { FAILED, PASSED, CONT, NEXT };

  TestModule() : _testIndex(0) {}
  virtual ~TestModule(void) {}
  virtual TestModule *Update(ISimHost *host, TextManagerState *state);
  TestModule &Step(
      std::function<TestStep(ISimHost *host, TextManagerState *)> step);
  TestModule &StepOnce(
      std::function<void(ISimHost *host, TextManagerState *)> step);

 protected:
  virtual void Setup(ISimHost *host) = 0;
  virtual TestModule *NextTestModule() = 0;

 private:
  std::vector<std::function<TestStep(ISimHost *, TextManagerState *)>> _steps;
  int _testIndex;
};

class Module : public IModule {
 public:
  virtual ~Module(void);
  Module();

  bool STNew(ISimHost *simHost, const wchar_t *workingFolder) override final;
  bool STDispose(ISimHost *simHost) override final;
  bool STUpdate(ISimHost *simHost, double elapsedTime) override final;
  void STShutDown(ISimHost *simHost) override final;

  bool RTBeforeFirstDraw(MGDF::IRenderHost *renderHost) override final;
  bool RTDraw(IRenderHost *renderHost, double alpha) override final;
  bool RTBeforeBackBufferChange(IRenderHost *renderHost) override final;
  bool RTBackBufferChange(IRenderHost *renderHost) override final;
  bool RTBeforeDeviceReset(IRenderHost *renderHost) override final;
  bool RTDeviceReset(IRenderHost *renderHost) override final;

  void Panic() override final;

 private:
  std::wstring _workingFolder;

  std::unique_ptr<TestModule> _testModule;
  BufferedGameState<TextManagerState> _stateBuffer;
  std::unique_ptr<TextManager> _textManager;
  ComObject<MGDF::IPerformanceCounter> _textManagerCounter;
  ComObject<MGDF::IPerformanceCounter> _testModuleCounter;
};

}  // namespace Test
}  // namespace MGDF
