#include "stdafx.h"

#include <algorithm>
#include <locale>

std::string GetJsonValue(const Json::Value &json, const std::string &name) {
  if (!json.isMember(name)) {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](char ch) {
      return std::tolower(ch, std::locale("en_US.utf8"));
    });
    return json[lower].asString();
  } else {
    return json[name].asString();
  }
}