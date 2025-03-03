#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
#include <unordered_map>

namespace MGDF {
namespace core {

class PreferenceSetImpl : public ComBase<IMGDFPreferenceSet> {
 public:
  virtual ~PreferenceSetImpl() {}
  std::unordered_map<std::string, std::string> Preferences;
};

}  // namespace core
}  // namespace MGDF
