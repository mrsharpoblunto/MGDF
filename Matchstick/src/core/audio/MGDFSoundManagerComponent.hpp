#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>

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