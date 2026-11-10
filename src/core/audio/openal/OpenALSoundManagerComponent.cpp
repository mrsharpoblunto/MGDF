#include "StdAfx.h"

#include "OpenALSoundManagerComponent.hpp"

#include "OpenALSoundManagerComponentImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace audio {
namespace openal_audio {

bool CreateSoundManagerComponent(bool hrtf, const std::filesystem::path &file,
                                 ComObject<ISoundManagerComponent> &comp) {
  const auto impl = MakeCom<OpenALSoundManagerComponentImpl>(
      AudioOptions{.Hrtf = hrtf, .OutputFile = file});
  const auto result = impl->Init();
  if (FAILED(result)) {
    return false;
  }
  comp = impl;
  return true;
}

}  // namespace openal_audio
}  // namespace audio
}  // namespace core
}  // namespace MGDF