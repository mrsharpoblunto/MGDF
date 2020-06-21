#pragma once

#include <MGDF/MGDF.h>
#include <string>

namespace MGDF {
namespace core {

class MGDFVersionInfo {
 public:
  virtual ~MGDFVersionInfo(){};
  static const char *MGDF_VERSION();
  static const INT32 MGDF_INTERFACE_VERSION = 1;
};

}  // namespace core
}  // namespace MGDF