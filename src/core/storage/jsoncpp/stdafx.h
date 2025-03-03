#pragma once

#include <fstream>
#include <iostream>
#include <nlohmann\/json.hpp>

#include "../../MGDFStdAfxBase.h"

std::string GetJsonValue(const nlohmann::json &json, const std::string &name);
