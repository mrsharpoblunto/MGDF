// stdafx.cpp : source file that includes just the standard includes
// core.common.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
char tolowerChar(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}
