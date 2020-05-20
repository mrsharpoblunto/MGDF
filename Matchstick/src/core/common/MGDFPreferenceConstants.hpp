#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.hpp>

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

template <>
std::string ToString(const ComObject<IString> &str);

template <typename T>
T FromString(const ComObject<IString> &str) {}

template <>
bool FromString(const ComObject<IString> &str);

template <>
int FromString(const ComObject<IString> &str);

template <>
long FromString(const ComObject<IString> &str);

template <>
unsigned int FromString(const ComObject<IString> &str);

template <>
unsigned long FromString(const ComObject<IString> &str);

template <>
double FromString(const ComObject<IString> &str);

template <>
float FromString(const ComObject<IString> &str);

}  // namespace core
}  // namespace MGDF
