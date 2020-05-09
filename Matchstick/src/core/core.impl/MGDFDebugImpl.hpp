#pragma once

#include <MGDF/MGDFDebug.hpp>
#include <atomic>
#include <map>
#include <sstream>
#include <string>

#include "MGDFHostStats.hpp"
#include "MGDFTimer.hpp"
#include "MGDFTextStream.hpp"

namespace MGDF {
namespace core {

class Debug : public IDebug {
 public:
  virtual ~Debug(){};
  Debug(Timer *timer);
  void Set(const char *section, const char *key,
           const char *value) override final;
  virtual void Clear(const char *section, const char *key) override final;
  virtual bool IsShown() const override final;
  virtual void ToggleShown() override final;

  void DumpInfo(const HostStats &stats, TextStream &ss) const;

 private:
  std::map<std::string, std::map<std::string, std::string>> _data;
  mutable std::atomic<bool> _shown;
  Timer *_timer;
};

}  // namespace core
}  // namespace MGDF
