#include "StdAfx.h"

#include "MGDFPreferenceConstants.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

const char *PreferenceConstants::MUSIC_VOLUME = "host.musicVolume";
const char *PreferenceConstants::SOUND_VOLUME = "host.soundVolume";
const char *PreferenceConstants::SIM_FPS = "host.simFps";
const char *PreferenceConstants::RENDER_FPS = "host.renderFps";
const char *PreferenceConstants::SCREEN_X = "host.screenX";
const char *PreferenceConstants::SCREEN_Y = "host.screenY";
const char *PreferenceConstants::FULL_SCREEN = "host.fullScreen";
const char *PreferenceConstants::FULL_SCREEN_EXCLUSIVE =
    "host.fullScreenExclusive";
const char *PreferenceConstants::MULTISAMPLE_LEVEL = "host.multiSampleLevel";
const char *PreferenceConstants::RT_MULTISAMPLE_LEVEL =
    "host.RTMultiSampleLevel";
const char *PreferenceConstants::VSYNC = "host.vsync";
const char *PreferenceConstants::WINDOW_RESIZE = "host.windowResize";
const char *PreferenceConstants::WINDOW_SIZEX = "host.windowSizeX";
const char *PreferenceConstants::WINDOW_SIZEY = "host.windowSizeY";
const char *PreferenceConstants::WINDOW_POSITIONX = "host.windowPositionX";
const char *PreferenceConstants::WINDOW_POSITIONY = "host.windowPositionY";

const std::string S_1("1");

template <>
bool FromString(const std::string &str) {
  auto rawVal = ToString(str);
  return rawVal == S_1;
}

template <>
int FromString(const std::string &str) {
  return atoi(str.c_str());
}

template <>
long FromString(const std::string &str) {
  return atol(str.c_str());
}

template <>
unsigned int FromString(const std::string &str) {
  return static_cast<unsigned int>(atol(str.c_str()));
}

template <>
unsigned long FromString(const std::string &str) {
  return static_cast<unsigned int>(atoll(str.c_str()));
}

template <>
double FromString(const std::string &str) {
  return atof(str.c_str());
}

template <>
float FromString(const std::string &str) {
  return static_cast<float>(FromString<double>(str));
}

bool GetPreference(ComObject<IGame> &game, const std::string &name,
                   std::string &value) {
  size_t size = 0;
  game->GetPreference(name.c_str(), nullptr, &size);
  value.resize(size);
  auto result =
      SUCCEEDED(game->GetPreference(name.c_str(), value.data(), &size));
  if (!result) {
    value.clear();
  }
  return result;
}
}  // namespace core
}  // namespace MGDF
