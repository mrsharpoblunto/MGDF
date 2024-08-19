#pragma once

// If app hasn't choosen, set to work with Windows 7 and beyond
#ifndef WINVER
#define WINVER 0x0A00
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// CRT's memory leak detection
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#endif

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <windows.h>

// Direct3D includes

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

#include <MGDF/MGDF.h>

constexpr auto TIMER_SAMPLES = 60;
