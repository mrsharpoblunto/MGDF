#pragma once
#include <filesystem>

#include "../MGDFSoundManagerComponent.hpp"

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

bool CreateSoundManagerComponent(bool hrtf, const std::filesystem::path &file,
                                 ComObject<ISoundManagerComponent> &comp);

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF