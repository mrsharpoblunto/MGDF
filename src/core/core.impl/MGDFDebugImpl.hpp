#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <map>
#include <mutex>
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
  void __stdcall SetHostRenderingEnabled(BOOL enabled) final;
  BOOL __stdcall IsHostRenderingEnabled() final;
  HRESULT __stdcall GetOverlayData(char *buffer, UINT64 *length) final;

  // the frame timings the overlay reads; owned by the app
  void SetMetrics(const HostMetrics *metrics);
  void DumpInfo(const HostMetrics &stats, TextStream &ss) const;

 private:
  std::string BuildOverlayData() const;

  // Set/Clear run on the sim thread while the overlay reads on the render
  // thread
  mutable std::mutex _dataMutex;
  std::map<std::string, std::map<std::string, std::string>> _data;
  mutable std::atomic<bool> _shown;
  std::atomic<bool> _hostRendering;
  Timer *_timer;
  const HostMetrics *_metrics;
};

}  // namespace core
}  // namespace MGDF
