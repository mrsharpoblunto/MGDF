#pragma once

#include <chrono>
#include <memory>

#include "AudioState.hpp"

namespace MGDF::core::audio::openal_audio {

class StreamWorker {
 public:
  explicit StreamWorker(std::shared_ptr<AudioControl> control);
  ~StreamWorker();
  HRESULT Create(IMGDFReadOnlyFile *file, IMGDFSoundStream **stream);
  void Stop();
  bool WaitForUpdate(std::chrono::milliseconds timeout);

 private:
  struct Impl;
  std::unique_ptr<Impl> _impl;
};

}  // namespace MGDF::core::audio::openal_audio
