#pragma once

#include <AL/alc.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <memory>

#include "../MGDFAudioCapture.hpp"

namespace MGDF::core::audio::openal_audio {

// The start of one endpoint block: its leading silence, and the play time of
// its first rendered frame.
struct OutputBlockStart {
  UINT32 Silence = 0;
  int64_t PlayTime = 0;
};

// time: the endpoint clock estimate for the block. through: the play time at
// which the rendered stream ends. An endpoint that has not joined the stream
// gets silence up to that end. A joined endpoint continues the stream: jitter
// in the clock estimate must not remove a frame.
OutputBlockStart StartOutputBlock(bool joined, int64_t time, int64_t through,
                                  UINT32 frames);

class AudioOutput {
 public:
  AudioOutput(ALCdevice *device, bool hrtf,
              const ComObject<AudioCaptureSource> &capture);
  ~AudioOutput();
  HRESULT Start();
  void Stop();
  void SetHrtf(bool enabled);
  std::atomic<UINT64> DeviceChanges{0};
  std::atomic<float> RenderMs{0};

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};
}  // namespace MGDF::core::audio::openal_audio
