#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>

#include <vector>
#include <string>

void ReadLines(const MGDF::ComObject<IMGDFFileReader> &reader,
               std::vector<std::string> &list);
