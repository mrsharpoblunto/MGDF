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

#include <json/json.h>

#include <fstream>
#include <iostream>

std::string GetJsonValue(const Json::Value &json, const std::string &name);

#include <MGDF/MGDF.h>
