#pragma once

// If app hasn't choosen, set to work with Windows 98, Windows Me, Windows 2000, Windows XP and beyond
#ifndef WINVER
#define WINVER         0x0410
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0410 
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT   0x0500 
#endif

// CRT's memory leak detection
#if defined(DEBUG) | defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

// Enable extra D3D debugging in debug builds if using the debug DirectX runtime.  
// This makes D3D objects work well in the debugger watch window, but slows down 
// performance slightly.
#if defined(DEBUG) | defined(_DEBUG)
#ifndef D3D_DEBUG_INFO
#define D3D_DEBUG_INFO
#endif
#endif

//#define AUTOLIB //to automatically include the libs needed for d3d
#ifdef AUTOLIB
#pragma comment( lib, "dxerr.lib" )
#pragma comment( lib, "dxguid.lib" )
#if defined(DEBUG) | defined(_DEBUG)
#pragma comment( lib, "d3dx9d.lib" )
#else
#pragma comment( lib, "d3dx9.lib" )
#endif
#pragma comment( lib, "d3d9.lib" )
#pragma comment( lib, "winmm.lib" )
#endif

// Direct3D includes
#include <d3d9.h>
#include <d3dx9.h>
#include <dxerr.h>

//enable easier shader debugging
#if defined(DEBUG) | defined(_DEBUG)
#ifndef D3DXSHADER_DEBUG
#define D3DXSHADER_DEBUG
#endif
#ifndef D3DXSHADER_SKIPOPTIMIZATION
#define D3DXSHADER_SKIPOPTIMIZATION
#endif
#endif

#include <MGDF/MGDF.hpp>

