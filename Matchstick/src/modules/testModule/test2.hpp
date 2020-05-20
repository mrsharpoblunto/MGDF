#pragma once

#include <time.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

class Test2 : public TestModule {
 public:
  virtual ~Test2(void);
  Test2();

  void Setup(ISimHost *host) override final;
  TestModule *NextTestModule() override final;

 private:
  INT32 _x, _y;
  ComObject<IInputManager> _input;
  ComObject<IVirtualFileSystem> _vfs;
  ComObject<ISoundManager> _soundManager;
  ComObject<ISound> _sound;
  ComObject<ISoundStream> _stream;
};

}  // namespace Test
}  // namespace MGDF
