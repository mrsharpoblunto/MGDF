#pragma once

#include <MGDF/MGDF.hpp>

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

/**
this class acts as a command console to the lua  interfaces
*/
class Test3 : public TestModule {
 public:
  virtual ~Test3(void);
  Test3();

  void Setup(ISimHost *host) override final;

  TestModule *NextTestModule() override final;

 private:
  ComObject<IInputManager> _input;
  ComObject<ISaveManager> _saves;
  ComObject<IGameState> _state;
  ComObject<IPendingSave> _pending;
};

}  // namespace Test
}  // namespace MGDF
