#include "stdafx.h"

#include <algorithm>
#include <locale>

std::string GetJsonValue(const nlohmann::json &json, const std::string &name) {
  auto found = json.find(name);
  if (found == json.cend()) {
    std::string lower(name);
    std::transform(lower.begin(), lower.end(), lower.begin(), [](char ch) {
      return std::tolower(ch, std::locale("en_US.utf8"));
    });
    found = json.find(lower);
    if (found == json.cend()) {
      return "";
    }
  }
  return found.value().get<std::string>();
}