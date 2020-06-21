#pragma once

#include <MGDF/MGDF.h>
#include <string>

namespace MGDF {
namespace core {

class VersionHelper {
 public:
  static MGDFVersion Create(const std::string &version);
  static std::string Format(const MGDFVersion &version);
  static INT32 Compare(const MGDFVersion &a, const MGDFVersion &b);
};

}  // namespace core
}  // namespace MGDF