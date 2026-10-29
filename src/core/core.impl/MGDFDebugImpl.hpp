#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <atomic>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include "MGDFHostMetrics.hpp"
#include "MGDFTimer.hpp"

namespace MGDF {
namespace core {

using DebugSections = std::map<std::string, std::map<std::string, std::string>>;

struct DebugFrameStats {
  bool HasLatency = false;
  double RenderLatency = 0.0;
  UINT64 RenderedFrames = 0;
  UINT64 PresentedFrames = 0;
};

// owns every buffer the MGDFDebugOverlayData pointers reference
class DebugOverlaySnapshot : public ComBase<IMGDFDebugOverlaySnapshot> {
 public:
  DebugOverlaySnapshot(const HostMetrics *metrics, const Timer *timer,
                       const DebugSections &sections,
                       const DebugFrameStats &frameStats);
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
  HRESULT __stdcall GetOverlaySnapshot(
      IMGDFDebugOverlaySnapshot **snapshot) final;

  // the frame timings the overlay reads; owned by the app
  void SetMetrics(const HostMetrics *metrics);
  void SetFrameStats(const DebugFrameStats &stats);

 private:
  // Set/Clear run on the sim thread while the overlay reads on the render
  // thread
  mutable std::mutex _dataMutex;
  DebugSections _data;
  DebugFrameStats _frameStats;
  Timer *_timer;
  const HostMetrics *_metrics;
};

}  // namespace core
}  // namespace MGDF
