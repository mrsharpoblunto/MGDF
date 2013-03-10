#include "StdAfx.h"
#include "MGDFPreferenceConstants.hpp"


#if defined(_DEBUG)
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#pragma warning(disable:4291)
#endif

namespace MGDF
{
namespace core
{

const char *PreferenceConstants::MUSIC_VOLUME = "musicVolume";
const char *PreferenceConstants::SOUND_VOLUME = "soundVolume";
const char *PreferenceConstants::RESOLUTION = "resolution";
const char *PreferenceConstants::SCREEN_X = "screenX";
const char *PreferenceConstants::SCREEN_Y = "screenY";
const char *PreferenceConstants::FULL_SCREEN = "fullScreen";
const char *PreferenceConstants::MULTISAMPLE_LEVEL = "multiSampleLevel";
const char *PreferenceConstants::RT_MULTISAMPLE_LEVEL = "RTMultiSampleLevel";
const char *PreferenceConstants::VSYNC = "vsync";
}
}
