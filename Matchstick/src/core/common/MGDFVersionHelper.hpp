#pragma once

#include <string>

namespace MGDF {
namespace core {

class VersionHelper {
 public:
  static Version Create(const std::string &version);
  static std::string Format(const Version &version);
  static INT32 Compare(const Version &a, const Version &b);
};

}  // namespace core
}  // namespace MGDF