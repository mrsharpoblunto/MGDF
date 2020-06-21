#pragma once

#include <time.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>

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

  void Setup(IMGDFSimHost *host) final;
  TestModule *NextTestModule() final;

 private:
  INT32 _x, _y;
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFVirtualFileSystem> _vfs;
  ComObject<IMGDFSoundManager> _soundManager;
  ComObject<IMGDFSound> _sound;
  ComObject<IMGDFSoundStream> _stream;
};

}  // namespace Test
}  // namespace MGDF
