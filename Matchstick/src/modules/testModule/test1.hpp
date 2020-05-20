#pragma once

#include <time.h>

#include <MGDF/MGDF.hpp>

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

class Test1 : public TestModule {
 public:
  virtual ~Test1(void);
  Test1();

  void Setup(ISimHost *host) override final;
  TestModule *NextTestModule() override final;

 private:
  bool _waitingForGamepad;
  LARGE_INTEGER _time;
  ComObject<IInputManager> _input;
  ComObject<IGamepad> _gamepad;
};

}  // namespace Test
}  // namespace MGDF
