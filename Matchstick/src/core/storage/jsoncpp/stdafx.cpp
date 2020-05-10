#include "stdafx.h"

#include <algorithm>

std::string GetJsonValue(const Json::Value &json, const std::string &name) {
  if (!json.isMember(name)) {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return json[lower].asString();
  } else {
    return json[name].asString();
  }
}