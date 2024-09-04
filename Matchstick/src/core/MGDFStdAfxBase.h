#pragma once

// If app hasn't choosen, set to work with Windows 10 and beyond
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
#include <MGDF/MGDF.h>
#include <WinSock2.h>
#include <assert.h>
#include <windows.h>
