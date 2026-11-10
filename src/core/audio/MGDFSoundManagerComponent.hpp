#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "MGDFAudioCapture.hpp"

namespace MGDF {
namespace core {
namespace audio {

class ISoundManagerComponent : public ComBase<IMGDFSoundManager> {
 public:
  virtual ~ISoundManagerComponent() {}
  virtual void Stop() = 0;
  virtual ComObject<AudioCaptureSource> CaptureSource() = 0;
};

}  // namespace audio
}  // namespace core
}  // namespace MGDF