#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <map>
#include <sstream>
#include <string>

#include "MGDFHostMetrics.hpp"
#include "MGDFTextStream.hpp"
#include "MGDFTimer.hpp"

namespace MGDF {
namespace core {

class Debug : public ComBase<IMGDFDebug> {
 public:
  virtual ~Debug() {};
  Debug(Timer *timer);
  void __stdcall Set(const char *section, const char *key,
                     const char *value) final;
  void __stdcall Clear(const char *section, const char *key) final;
  BOOL __stdcall IsShown() final;
  void __stdcall ToggleShown() final;

  void DumpInfo(const HostMetrics &stats, TextStream &ss) const;

 private:
  std::map<std::string, std::map<std::string, std::string>> _data;
  mutable std::atomic<bool> _shown;
  Timer *_timer;
};

}  // namespace core
}  // namespace MGDF
