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
class LoadSaveTests : public TestModule {
 public:
  virtual ~LoadSaveTests(void);
  LoadSaveTests();

  void Setup(IMGDFSimHost *host) final;

 private:
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFSaveManager> _saves;
  ComObject<IMGDFGameState> _state;
  ComObject<IMGDFWriteableVirtualFileSystem> _pending;
};

}  // namespace Test
}  // namespace MGDF
