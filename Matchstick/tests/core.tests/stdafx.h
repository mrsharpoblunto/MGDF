#pragma once

// If app hasn't choosen, set to work with Windows 7 and beyond
#ifndef WINVER
#define WINVER 0x0601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif

#include <stdlib.h>

// CRT's memory leak detection
#if defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#pragma warning(push)
#pragma warning(disable : 4996)
#include <xutility>
#pragma warning(pop)

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <UnitTest++.h>
#include <assert.h>
#include <windows.h>

#include <MGDF/MGDF.hpp>
#define ROWTEST(testtype, testname, ...) \
  TEST(RT_##testtype##_##testname) {     \
    testtype __x __VA_ARGS__;            \
    __x();                               \
  }
#define CHECK_WS_EQUAL(a, b) CHECK(std::wstring(a) == std::wstring(b))
