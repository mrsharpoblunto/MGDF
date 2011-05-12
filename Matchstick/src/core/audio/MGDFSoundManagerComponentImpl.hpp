#pragma once

#include "../common/MGDFComponents.hpp"

#ifdef USE_OPENAL_AUDIO
#include "openal/OpenALSoundManagerComponent.hpp"
#endif

namespace MGDF { namespace core { namespace audio {

#ifdef USE_OPENAL_AUDIO
	#define CreateSoundManagerComponentImpl openal_audio::CreateOpenALSoundManagerComponent
#endif

}}}