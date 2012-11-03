#pragma once

// If app hasn't choosen, set to work with Windows vista and beyond
#ifndef WINVER
#define WINVER         0x0600
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT   0x0600
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

#define AUTOLIB //to automatically include the libs needed for d3d
#ifdef AUTOLIB
#pragma comment( lib, "d3d11.lib" )
#endif

//some useful macro's to make deleting pointers easier
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

#include <string>
