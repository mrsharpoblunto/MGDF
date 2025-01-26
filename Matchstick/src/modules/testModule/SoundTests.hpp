#pragma once

#include <MGDF/MGDF.h>
#include <time.h>

#include <MGDF/ComObject.hpp>

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

class SoundTests : public TestModule {
 public:
  virtual ~SoundTests(void);
  SoundTests();

  void Setup(IMGDFSimHost *host) final;

 private:
  INT32 _x, _y;
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFReadOnlyVirtualFileSystem> _vfs;
  ComObject<IMGDFSoundManager> _soundManager;
  ComObject<IMGDFSound> _sound;
  ComObject<IMGDFSoundStream> _stream;
};

}  // namespace Test
}  // namespace MGDF
