#pragma once

#include <MGDF/MGDFSoundManager.hpp>
#include <MGDF/MGDFVirtualFileSystem.hpp>

#include "../common/MGDFSystemComponent.hpp"

namespace MGDF {
namespace core {
namespace audio {

class ISoundManagerComponent : public ISystemComponent, public ISoundManager {
 public:
  virtual ~ISoundManagerComponent() {}
  virtual void Update() = 0;
};

}  // namespace audio
}  // namespace core
}  // namespace MGDF