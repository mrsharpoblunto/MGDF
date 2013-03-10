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
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

// Direct3D includes

#define STRINGIFY(x) #x

#if defined(_DEBUG)
        #ifndef HR
        #define HR(x)													\
        {																\
                HRESULT hr = x;											\
                if(FAILED(hr))											\
                {														\
					std::string sender = "DirectX API call failed - ";	\
					sender += __FILE__;									\
					sender += ":";										\
					sender += STRINGIFY(__LINE__);						\
                    OutputDebugString(sender.c_str());					\
                }														\
        }
        #endif

#else
        #ifndef HR
        #define HR(x) x;
        #endif
#endif 

#include <MGDF/MGDF.hpp>

#define TIMER_SAMPLES 60

//some useful macro's to make deleting pointers easier
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { delete (p);     (p)=nullptr; }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=nullptr; }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=nullptr; } }
#endif
