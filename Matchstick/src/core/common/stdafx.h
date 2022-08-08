#pragma once

// If app hasn't choosen, set to work with Windows 7 and beyond
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
///
#include <WinSock2.h>

// CRT's memory leak detection
#if defined(DEBUG) | defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#include <stdlib.h>
#endif

#pragma warning(push)
#pragma warning(disable : 4996)
#include <xutility>
#pragma warning(pop)

#include <MGDF/MGDF.h>
