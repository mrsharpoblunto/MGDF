#pragma once

#include "../MGDFSoundManagerComponent.hpp"

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

bool CreateSoundManagerComponent(
    const ComObject<IMGDFReadOnlyVirtualFileSystem> &vfs,
    ComObject<ISoundManagerComponent> &comp);

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF