#pragma once

#include <AL/alc.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <memory>

#include "../MGDFAudioCapture.hpp"

namespace MGDF::core::audio::openal_audio {

class AudioOutput {
 public:
  AudioOutput(ALCdevice *device, bool hrtf,
              const ComObject<AudioCaptureSource> &capture);
  ~AudioOutput();
  HRESULT Start();
  void Stop();
  std::atomic<UINT64> DeviceChanges{0};
  std::atomic<float> RenderMs{0};

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};
}  // namespace MGDF::core::audio::openal_audio
