#pragma once

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDFDebug.hpp>
#include <atomic>
#include <map>
#include <sstream>
#include <string>

#include "MGDFHostStats.hpp"
#include "MGDFTextStream.hpp"
#include "MGDFTimer.hpp"

namespace MGDF {
namespace core {

class Debug : public ComBase<IDebug> {
 public:
  virtual ~Debug(){};
  Debug(Timer *timer);
  void Set(const char *section, const char *key, const char *value) final;
  void Clear(const char *section, const char *key) final;
  bool IsShown() const final;
  void ToggleShown() final;

  void DumpInfo(const HostStats &stats, TextStream &ss) const;

 private:
  std::map<std::string, std::map<std::string, std::string>> _data;
  mutable std::atomic<bool> _shown;
  Timer *_timer;
};

}  // namespace core
}  // namespace MGDF
