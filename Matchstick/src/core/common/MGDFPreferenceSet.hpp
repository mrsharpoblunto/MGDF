#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDFGame.hpp>
#include <unordered_map>

namespace MGDF {
namespace core {

class PreferenceSetImpl : public ComBase<IPreferenceSet> {
 public:
  virtual ~PreferenceSetImpl() {}
  std::unordered_map<std::string, std::string> Preferences;
};

}  // namespace core
}  // namespace MGDF
