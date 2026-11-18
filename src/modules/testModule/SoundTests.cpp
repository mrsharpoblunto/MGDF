#include "StdAfx.h"

#include "SoundTests.hpp"
namespace MGDF::Test {
SoundTests::~SoundTests() {
  if (_voice && _commands) {
    _commands->Stop(_voice, 0);
    _manager->Submit(_commands);
  }
}
void SoundTests::Setup(IMGDFSimHost *host) {
  host->GetSound(_manager.Assign());
  host->GetInput(_input.Assign());
  ComObject<IMGDFReadOnlyVirtualFileSystem> vfs;
  host->GetVFS(vfs.Assign());
  ComObject<IMGDFReadOnlyFile> file;
  if (_manager && vfs->GetFile(L"chimes.wav", file.Assign())) {
    _manager->CreateCommandList(_commands.Assign());
    const MGDFSoundSegment segment{0, 0, TRUE};
    const MGDFSoundSamplesDesc desc{&segment, 1};
    _manager->CreateSoundSamples(file, &desc, _samples.Assign());
  }
  Step([this](auto state) {
    state->Text.AddLine("Loading chimes.wav");
    if (!_samples || !_commands || _samples->HasFailed())
      return state->Fail("Cannot load audio samples");
    return _samples->IsLoaded() ? TestStep::PASSED : TestStep::CONT;
  })
      .StepOnce([this](auto state) {
        MGDFVoiceDesc voice{};
        voice.Bus = _manager->GetBus(MGDF_BUS_SOUND);
        voice.Volume = 1;
        voice.Pitch = 1;
        voice.GainHF = 1;
        voice.Spatial = TRUE;
        _voice = _commands->Play(_samples, &voice);
        _manager->Submit(_commands);
        state->Text.AddLine(
            "Use the arrow keys to move the sound. Press H to switch headphone "
            "rendering (HRTF). Press Y or N to record the result.");
      })
      .Step([this](auto state) {
        if (_input->IsKeyPress('Y') || _input->IsKeyPress('N')) {
          _commands->Stop(_voice, 0.05f);
          _manager->Submit(_commands);
          _voice = 0;
          return _input->IsKeyPress('Y')
                     ? TestStep::PASSED
                     : state->Fail("Sound did not pan with its position");
        }
        if (_input->IsKeyPress('H')) {
          _manager->SetHRTFEnabled(!_manager->GetHRTFEnabled());
          state->Text.AddLine(_manager->GetHRTFEnabled()
                                  ? "Headphone rendering on"
                                  : "Headphone rendering off");
        }
        if (_input->IsKeyDown(VK_LEFT)) _position.x -= 0.1f;
        if (_input->IsKeyDown(VK_RIGHT)) _position.x += 0.1f;
        if (_input->IsKeyDown(VK_UP)) _position.z -= 0.1f;
        if (_input->IsKeyDown(VK_DOWN)) _position.z += 0.1f;
        const MGDFSoundPosition velocity{};
        _commands->SetSpatial(_voice, &_position, &velocity, 0, 0.02f);
        _manager->Submit(_commands);
        return TestStep::CONT;
      })
      .Step([this, host](auto state) {
        ComObject<IMGDFRecorder> recorder;
        if (FAILED(host->GetRecorder(recorder.Assign())) ||
            FAILED(recorder->StartVideo("testmodule-sound",
                                        _recording.Assign())))
          return state->Fail("Cannot start a video recording");
        MGDFVoiceDesc voice{};
        voice.Bus = _manager->GetBus(MGDF_BUS_SOUND);
        voice.Volume = 1;
        voice.Pitch = 1;
        voice.GainHF = 1;
        _voice = _commands->Play(_samples, &voice);
        _manager->Submit(_commands);
        _recordEnd = _manager->GetAudioTime() + 3;
        state->Text.AddLine("Recording a 3 second video with sound");
        return TestStep::NEXT;
      })
      .Step([this](auto) {
        if (_manager->GetAudioTime() < _recordEnd) return TestStep::CONT;
        _commands->Stop(_voice, 0.05f);
        _manager->Submit(_commands);
        _voice = 0;
        _recording->Stop();
        return TestStep::NEXT;
      })
      .Step([this](auto state) {
        const auto result = _recording->GetResult();
        if (result == E_PENDING) return TestStep::CONT;
        if (FAILED(result)) return state->Fail("Video recording failed");
        UINT64 length = 0;
        _recording->GetPath(nullptr, &length);
        std::wstring path(length, L'\0');
        _recording->GetPath(path.data(), &length);
        std::string utf8(
            ::WideCharToMultiByte(CP_UTF8, 0, path.data(),
                                  static_cast<int>(path.size()), nullptr, 0,
                                  nullptr, nullptr),
            '\0');
        ::WideCharToMultiByte(CP_UTF8, 0, path.data(),
                              static_cast<int>(path.size()), utf8.data(),
                              static_cast<int>(utf8.size()), nullptr, nullptr);
        state->Text.AddLine("Play " + utf8);
        state->Text.AddLine(
            "Press Y if the video has the chimes sound, or N if not.");
        return TestStep::NEXT;
      })
      .Step([this](auto state) {
        if (_input->IsKeyPress('Y')) return TestStep::PASSED;
        if (_input->IsKeyPress('N'))
          return state->Fail("The recorded video has no sound");
        return TestStep::CONT;
      });
}
}  // namespace MGDF::Test
