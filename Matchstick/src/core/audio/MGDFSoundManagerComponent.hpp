#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

namespace MGDF {
namespace core {
namespace audio {

class ISoundManagerComponent : public ComBase<ISoundManager> {
 public:
  virtual ~ISoundManagerComponent() {}
  virtual void Update() = 0;
};

}  // namespace audio
}  // namespace core
}  // namespace MGDF