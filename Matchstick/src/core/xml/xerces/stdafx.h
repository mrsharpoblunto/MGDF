#pragma once

// CRT's memory leak detection
#if defined(DEBUG) | defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <iostream>
#include <fstream>
#include <xercesc/util/XMLUni.hpp>
#include <xercesc/sax2/attributes.hpp>

#include <MGDF/MGDF.hpp>
