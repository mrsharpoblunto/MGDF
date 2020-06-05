#pragma once

// If app hasn't choosen, set to work with Windows 7 and beyond
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

// CRT's memory leak detection
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <assert.h>
#include <windows.h>

// some useful macro's to make deleting pointers easier
#ifndef SAFE_DELETE
#define SAFE_DELETE(p) \
  {                    \
    delete (p);        \
    (p) = nullptr;     \
  }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) \
  {                          \
    delete[](p);             \
    (p) = nullptr;           \
  }
#endif

// other useful includes used commonly throughout the program
// STL strings and common container types
#include <list>
#include <sstream>
#include <string>
#include <vector>

#define FATALERROR(obj, msg)                             \
  {                                                      \
    std::ostringstream ss;                               \
    ss << __FILE__ << ':' << __LINE__;                   \
    std::ostringstream ms;                               \
    ms << msg;                                           \
    obj->FatalError(ss.str().c_str(), ms.str().c_str()); \
  }
