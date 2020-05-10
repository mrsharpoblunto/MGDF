#pragma once

#include <MGDF/MGDF.hpp>
#include <string>

namespace MGDF {
namespace core {

/**
Translates common error codes to more readable descriptions
*/
class Win32Exception {
 public:
  static std::string TranslateError(unsigned code);
};

}  // namespace core
}  // namespace MGDF
