#pragma once

#include <MGDF/MGDF.h>
#include <time.h>

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

class InputTests : public TestModule {
 public:
  virtual ~InputTests(void);
  InputTests();

  void Setup(IMGDFSimHost *host) final;

 private:
  bool _waitingForGamepad;
  LARGE_INTEGER _time;
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFGamepad> _gamepad;
  ComObject<IMGDFTimer> _timer;
};

}  // namespace Test
}  // namespace MGDF
