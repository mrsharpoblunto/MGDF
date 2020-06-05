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

  void Setup(ISimHost *host) final;
  TestModule *NextTestModule() final;

 private:
  bool _waitingForGamepad;
  LARGE_INTEGER _time;
  ComObject<IInputManager> _input;
  ComObject<IGamepad> _gamepad;
  ComObject<ITimer> _timer;
};

}  // namespace Test
}  // namespace MGDF
