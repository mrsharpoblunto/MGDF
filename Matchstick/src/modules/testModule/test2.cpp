#include "StdAfx.h"

#include "Test2.hpp"

#include "Test3.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace Test {

Test2::~Test2(void) {}

Test2::Test2() : _x(0), _y(0) {}

TestModule *Test2::NextTestModule() { return new Test3(); }

void Test2::Setup(ISimHost *host) {
  host->GetInput(_input.Assign());
  host->GetVFS(_vfs.Assign());
  host->GetSound(_soundManager.Assign());

  Step([this](auto state) {
    state->AddLine("");
    state->AddLine("SoundManager Tests");
    state->AddLine("");
    state->AddLine("Checking SoundManager is initialized");
    return _soundManager ? TestStep::PASSED : TestStep::FAILED;
  })
      .Step([this](auto state) {
        state->AddLine("Loading sound chimes.wav");
        _soundManager->SetEnableAttenuation(true);
        ComObject<IFile> file;
        if (_vfs->GetFile(L"chimes.wav", file.Assign()) &&
            MGDF_OK != _soundManager->CreateSound(file, 0, _sound.Assign())) {
          return TestStep::FAILED;
        } else {
          ComObject<ISound> s;
          if (S_OK != _sound->QueryInterface(__uuidof(MGDF::ISound),
                                             (void **)s.Assign())) {
            return TestStep::FAILED;
          } else {
            return TestStep::PASSED;
          }
        }
      })
      .StepOnce([this](auto state) {
        _sound->SetLooping(true);
        _sound->SetInnerRange(0);
        _sound->SetOuterRange(250);
        _sound->Play();
        state->AddLine("Is a sound playing? [Y/N]");
      })
      .Step([this](auto state) {
        (void)state;
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return TestStep::FAILED;
        } else {
          return TestStep::CONT;
        }
      })
      .StepOnce([this](auto state) {
        state->AddLine(
            "Use arrow keys to change sounds position, press [Y/N] if the "
            "sound "
            "adjusts accordingly");
      })
      .Step([this](auto state) {
        (void)state;
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          _sound.Clear();
          return TestStep::FAILED;
        } else {
          SoundPosition position;
          _sound->GetPosition(&position);
          if (_input->IsKeyDown(VK_UP)) {
            position.y += 1;
          }
          if (_input->IsKeyDown(VK_DOWN)) {
            position.y -= 1;
          }
          if (_input->IsKeyDown(VK_LEFT)) {
            position.x -= 1;
          }
          if (_input->IsKeyDown(VK_RIGHT)) {
            position.x += 1;
          }
          _sound->SetPosition(&position);
          return TestStep::CONT;
        }
      })
      .Step([this](auto state) {
        _sound.Clear();
        state->AddLine("Loading stream stream.ogg");
        ComObject<IFile> file;
        if (_vfs->GetFile(L"Stream.ogg", file.Assign()) &&
            MGDF_OK !=
                _soundManager->CreateSoundStream(file, _stream.Assign())) {
          return TestStep::FAILED;
        } else {
          ComObject<ISoundStream> ss;
          if (S_OK != _stream->QueryInterface(__uuidof(MGDF::ISoundStream),
                                              (void **)ss.Assign())) {
            return TestStep::FAILED;
          } else if (MGDF_OK != _stream->Play()) {
            return TestStep::FAILED;
          } else {
            return TestStep::PASSED;
          }
        }
      })
      .StepOnce([](auto state) {
        state->AddLine(
            "Playing stream, press [Y/N] if the stream is actually playing");
      })
      .Step([this](auto state) {
        (void)state;
        if (_input->IsKeyPress('Y')) {
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          return TestStep::FAILED;
        } else {
          return TestStep::CONT;
        }
      })
      .StepOnce([](auto state) {
        state->AddLine(
            "Use [P] to toggle pause/play, press [Y/N] if this is working.");
      })
      .Step([this](auto state) {
        (void)state;
        if (_input->IsKeyPress('Y')) {
          _stream.Clear();
          return TestStep::PASSED;
        } else if (_input->IsKeyPress('N')) {
          _stream.Clear();
          return TestStep::FAILED;
        } else {
          if (_input->IsKeyPress('P')) {
            if (_stream->IsPaused()) {
              if (MGDF_OK != _stream->Play()) {
                _stream.Clear();
                return TestStep::FAILED;
              }
            } else
              _stream->Pause();
          }
          return TestStep::CONT;
        }
      });
}
}  // namespace Test
}  // namespace MGDF