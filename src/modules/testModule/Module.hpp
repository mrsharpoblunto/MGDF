#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/D3D12CommandList.hpp>
#include <array>
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

enum class TestStep { FAILED, PASSED, CONT, NEXT };

class TestState {
 public:
  TestState() {}
  TestState(const TestState &state);
  TestState(const TestState &startState, const TestState &endState,
            double alpha);
  virtual ~TestState() {}
  TextManagerState Text;

  // fails the current test, recording the reason for the assertion failure
  // so it can be displayed below the failed test
  TestStep Fail(const std::string &error) {
    LastError = error;
    return TestStep::FAILED;
  }
  std::string LastError;
};

class TestModule {
 public:
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
  void __stdcall RTAfterPresent(IMGDFRenderHost *host) final;
  BOOL __stdcall RTBeforeBackBufferChange(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTBackBufferChange(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTBeforeDeviceReset(IMGDFRenderHost *renderHost) final;
  BOOL __stdcall RTDeviceReset(IMGDFRenderHost *renderHost) final;
  void __stdcall RTShutDown(IMGDFRenderHost *renderHost) final;
  void __stdcall Panic() final;

 private:
  bool InitD3D12(IMGDFRenderHost *host);
  bool DrawD3D12(IMGDFRenderHost *host, double elapsed);
  void ReleaseD3D12();
  ComObject<ID3D12Device10> _d3d12Device;
  ComObject<ID3D12CommandQueue> _directQueue;
  ComObject<ID3D12Fence> _frameFence;
  std::array<ComObject<ID3D12CommandAllocator>, 2> _allocators;
  std::array<D3D12CommandList, 2> _lists;
  std::array<ComObject<IMGDFPerformanceCounter>, 2> _gpuCounters;
  std::array<UINT64, 2> _slotFences{};
  double _renderTime = 0;
  TestResults _results;
  bool _finalResult;
  bool _awaitingPresent = false;
  UINT64 _presentedFrames = 0;
  std::list<std::unique_ptr<TestModule>> _testModules;
  std::list<std::unique_ptr<TestModule>>::iterator _currentModule;
  BufferedGameState<TestState> _stateBuffer;
  std::unique_ptr<TextManager> _textManager;
  ComObject<IMGDFPerformanceCounter> _textManagerCounter;
  ComObject<IMGDFPerformanceCounter> _testModuleCounter;
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFRenderSettingsManager> _renderSettings;
  bool _draggingScrollbar;
  INT32 _wheelAccumulator;
};

}  // namespace Test
}  // namespace MGDF
