#pragma once

#include "../MGDFStdAfxBase.h"

#define FATALERROR(obj, msg)                             \
  {                                                      \
    std::ostringstream ss;                               \
    ss << __FILE__ << ':' << __LINE__;                   \
    std::ostringstream ms;                               \
    ms << msg;                                           \
    obj->FatalError(ss.str().c_str(), ms.str().c_str()); \
  }

constexpr auto TIMER_SAMPLES = 60;
constexpr auto GPU_TIMER_BUFFER = 8;
