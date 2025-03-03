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
class DisplayTests : public TestModule {
 public:
  virtual ~DisplayTests(void);
  DisplayTests();

  void Setup(IMGDFSimHost *host) final;

 private:
  ComObject<IMGDFInputManager> _input;
  ComObject<IMGDFRenderSettingsManager> _settings;
};

}  // namespace Test
}  // namespace MGDF
