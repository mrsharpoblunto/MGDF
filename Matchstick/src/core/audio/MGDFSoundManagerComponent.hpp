#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

namespace MGDF {
namespace core {
namespace audio {

class ISoundManagerComponent : public ComBase<IMGDFSoundManager> {
 public:
  virtual ~ISoundManagerComponent() {}
  virtual void Update() = 0;
};

}  // namespace audio
}  // namespace core
}  // namespace MGDF