#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "MGDFHostMetrics.hpp"
#include "MGDFTextStream.hpp"
#include "MGDFTimer.hpp"

namespace MGDF {
namespace core {

using DebugSections =
    std::map<std::string, std::map<std::string, std::string>>;

// owns every buffer the MGDFDebugOverlayData pointers reference
class DebugOverlaySnapshot : public ComBase<IMGDFDebugOverlaySnapshot> {
 public:
  DebugOverlaySnapshot(const HostMetrics *metrics, const Timer *timer,
                       const DebugSections &sections);
  virtual ~DebugOverlaySnapshot() {}

  const MGDFDebugOverlayData *__stdcall GetData() final { return &_data; }

 private:
  static MGDFDebugTiming View(double average,
                              const std::vector<double> &samples);

  TimingSamples _samples;
  std::vector<CounterSnapshot> _counters;
  std::vector<MGDFDebugCounter> _counterViews;
  DebugSections _sections;
  std::vector<MGDFDebugEntry> _entries;
  MGDFDebugOverlayData _data;
};

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
  HRESULT __stdcall GetOverlaySnapshot(
      IMGDFDebugOverlaySnapshot **snapshot) final;

  // the frame timings the overlay reads; owned by the app
  void SetMetrics(const HostMetrics *metrics);
  void DumpInfo(const HostMetrics &stats, TextStream &ss) const;

 private:
  // Set/Clear run on the sim thread while the overlay reads on the render
  // thread
  mutable std::mutex _dataMutex;
  DebugSections _data;
  mutable std::atomic<bool> _shown;
  std::atomic<bool> _hostRendering;
  Timer *_timer;
  const HostMetrics *_metrics;
};

}  // namespace core
}  // namespace MGDF
