#pragma once

#include <json/json.h>

#include <fstream>
#include <iostream>

#include "../../MGDFStdAfxBase.h"

std::string GetJsonValue(const Json::Value &json, const std::string &name);
