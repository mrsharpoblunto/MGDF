#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>
#include <sstream>

#include "MGDFResources.hpp"

namespace MGDF {
namespace core {

class PreferenceConstants {
 public:
  static const char *SCREEN_X;
  static const char *SCREEN_Y;
  static const char *MUSIC_VOLUME;
  static const char *SOUND_VOLUME;
  static const char *SIM_FPS;
  static const char *RENDER_FPS;
  static const char *FULL_SCREEN;
  static const char *FULL_SCREEN_EXCLUSIVE;
  static const char *MULTISAMPLE_LEVEL;
  static const char *RT_MULTISAMPLE_LEVEL;
  static const char *VSYNC;
  static const char *WINDOW_RESIZE;
  static const char *WINDOW_SIZEX;
  static const char *WINDOW_SIZEY;
  static const char *WINDOW_POSITIONX;
  static const char *WINDOW_POSITIONY;
};

template <typename T>
std::string ToString(const T &val) {
  std::ostringstream ss;
  ss << val;
  return ss.str();
}

template <typename T>
T FromString(const std::string &str) {}

template <>
bool FromString(const std::string &str);

template <>
int FromString(const std::string &str);

template <>
long FromString(const std::string &str);

template <>
unsigned int FromString(const std::string &str);

template <>
unsigned long FromString(const std::string &str);

template <>
double FromString(const std::string &str);

template <>
float FromString(const std::string &str);

bool GetPreference(ComObject<IGame> &game, const std::string &name,
                   std::string &value);

}  // namespace core
}  // namespace MGDF
