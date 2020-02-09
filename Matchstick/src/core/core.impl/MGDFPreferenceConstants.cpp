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

const char *PreferenceConstants::MUSIC_VOLUME = "host.musicVolume";
const char *PreferenceConstants::SOUND_VOLUME = "host.soundVolume";
const char *PreferenceConstants::SIM_FPS = "host.simFps";
const char *PreferenceConstants::RENDER_FPS = "host.renderFps";
const char *PreferenceConstants::SCREEN_X = "host.screenX";
const char *PreferenceConstants::SCREEN_Y = "host.screenY";
const char *PreferenceConstants::FULL_SCREEN = "host.fullScreen";
const char *PreferenceConstants::FULL_SCREEN_EXCLUSIVE = "host.fullScreenExclusive";
const char *PreferenceConstants::MULTISAMPLE_LEVEL = "host.multiSampleLevel";
const char *PreferenceConstants::RT_MULTISAMPLE_LEVEL = "host.RTMultiSampleLevel";
const char *PreferenceConstants::VSYNC = "host.vsync";
const char *PreferenceConstants::WINDOW_RESIZE = "host.windowResize";
const char *PreferenceConstants::WINDOW_SIZEX = "host.windowSizeX";
const char *PreferenceConstants::WINDOW_SIZEY = "host.windowSizeY";

}
}
