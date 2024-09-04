#pragma once

#include "MGDFStdAfxBase.h"

#define STRINGIFY(x) #x

#if defined(_DEBUG)
#ifndef HR
#define HR(x)                                            \
  {                                                      \
    HRESULT hr = x;                                      \
    if (FAILED(hr)) {                                    \
      std::string sender = "DirectX API call failed - "; \
      sender += __FILE__;                                \
      sender += ":";                                     \
      sender += STRINGIFY(__LINE__);                     \
      OutputDebugString(sender.c_str());                 \
    }                                                    \
  }
#endif

#else
#ifndef HR
#define HR(x) x;
#endif
#endif

#define FATALERROR(obj, msg)                             \
  {                                                      \
    std::ostringstream ss;                               \
    ss << __FILE__ << ':' << __LINE__;                   \
    std::ostringstream ms;                               \
    ms << msg;                                           \
    obj->FatalError(ss.str().c_str(), ms.str().c_str()); \
  }

constexpr auto TIMER_SAMPLES = 60;
