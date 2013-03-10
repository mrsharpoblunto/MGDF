#pragma once

// CRT's memory leak detection
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#pragma warning(disable:4291)
#endif

#pragma warning( push )
#pragma warning( disable:4996 )
#include <xutility>
#pragma warning ( pop )

#include <MGDF/MGDF.hpp>