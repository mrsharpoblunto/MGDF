#pragma once
#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "Module.hpp"
namespace MGDF::Test {
class SoundTests : public TestModule {
 public:
  ~SoundTests();
  SoundTests() = default;
  void Setup(IMGDFSimHost *host) final;

 private:
  ComObject<IMGDFSoundManager> _manager;
  ComObject<IMGDFSoundCommandList> _commands;
  ComObject<IMGDFSoundSamples> _samples;
  ComObject<IMGDFInputManager> _input;
  MGDFVoiceId _voice = 0;
  MGDFSoundPosition _position{};
};
}  // namespace MGDF::Test
