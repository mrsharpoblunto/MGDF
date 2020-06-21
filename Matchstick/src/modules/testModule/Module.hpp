#pragma once

#include <time.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
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
  virtual TestModule *Update(IMGDFSimHost *host, TextManagerState *state);
  TestModule &Step(std::function<TestStep(TextManagerState *)> step);
  TestModule &StepOnce(std::function<void(TextManagerState *)> step);

 protected:
  virtual void Setup(IMGDFSimHost *host) = 0;
  virtual TestModule *NextTestModule() = 0;

 private:
  std::vector<std::function<TestStep(TextManagerState *)>> _steps;
  int _testIndex;
};

class Module : public ComBase<IMGDFModule> {
 public:
  virtual ~Module(void);
  Module();

  BOOL __stdcall STNew(IMGDFSimHost *simHost, const wchar_t *workingFolder) final;
  BOOL __stdcall STUpdate(IMGDFSimHost *simHost, double elapsedTime) final;
  void __stdcall STShutDown(IMGDFSimHost *simHost) final;
  BOOL __stdcall RTBeforeFirstDraw(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTDraw(IMGDFRenderHost *renderHost, double alpha) final;
  BOOL __stdcall RTBeforeBackBufferChange(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTBackBufferChange(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTBeforeDeviceReset(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTDeviceReset(IMGDFRenderHost *renderHost) final;
  void __stdcall Panic() final;

 private:
  std::wstring _workingFolder;

  std::unique_ptr<TestModule> _testModule;
  BufferedGameState<TextManagerState> _stateBuffer;
  std::unique_ptr<TextManager> _textManager;
  ComObject<IMGDFPerformanceCounter> _textManagerCounter;
  ComObject<IMGDFPerformanceCounter> _testModuleCounter;
};

}  // namespace Test
}  // namespace MGDF
