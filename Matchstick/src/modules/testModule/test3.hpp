#pragma once

#include <MGDF/MGDF.h>

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

  void Setup(IMGDFSimHost *host) final;

  TestModule *NextTestModule() final;

 private:
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFSaveManager> _saves;
  ComObject<IMGDFGameState> _state;
  ComObject<IMGDFPendingSave> _pending;
};

}  // namespace Test
}  // namespace MGDF
