#pragma once

// If app hasn't choosen, set to work with Windows 7 and beyond
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <stdlib.h>

// CRT's memory leak detection
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <windows.h>

#include <MGDF/MGDF.h>

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
