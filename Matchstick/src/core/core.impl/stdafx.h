#pragma once

// If app hasn't choosen, set to work with Windows vista and beyond
#ifndef WINVER
#define WINVER         0x0600
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT   0x0600
#endif

#include <stdlib.h>

// CRT's memory leak detection
#if defined(DEBUG) | defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

#include <MGDF/MGDF.hpp>

#define TIMER_SAMPLES 60
#define GPU_TIMER_BUFFER 3

