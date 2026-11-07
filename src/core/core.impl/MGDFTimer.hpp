#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>

namespace MGDF {
namespace core {

class Timer;
class D3D12CounterManager;

// a counter's recent history, as the debug overlay reads it
struct CounterSnapshot {
  std::string Name;
  bool GPU = false;
  double Average = 0.0;
  std::vector<double> Samples;
};

class CounterBase : public ComBase<IMGDFPerformanceCounter> {
 public:
  static constexpr const size_t MaxSamples = 5;

  virtual ~CounterBase();
  CounterBase(IMGDFMetric *metric, Timer &timer);

  void __stdcall GetMetric(IMGDFMetric **metric) final;
  HRESULT __stdcall Begin(const MGDFTags *tags,
                          IMGDFPerformanceCounterScope **scope) final;

  double __stdcall GetAverageValue() final;

  void Snapshot(CounterSnapshot &snapshot, bool gpu);

 protected:
  void AddSample(double sample);
  virtual HRESULT DoBegin(std::map<std::string, std::string> &tags,
                          IMGDFPerformanceCounterScope **scope) = 0;
  Timer &_timer;
  ComObject<IMGDFMetric> _metric;
  std::deque<double> _samples;
  double _average;
  std::mutex _mutex;
};

class CPUPerformanceCounter;

class CPUPerformanceCounterScope
    : public ComBase<IMGDFPerformanceCounterScope> {
 public:
  CPUPerformanceCounterScope(CPUPerformanceCounter *counter,
                             std::map<std::string, std::string> &tags);
  virtual ~CPUPerformanceCounterScope();

 private:
  std::map<std::string, std::string> _tags;
  CPUPerformanceCounter *_counter;
  LARGE_INTEGER _start;
};

class CPUPerformanceCounter : public CounterBase {
 public:
  friend class CPUPerformanceCounterScope;
  virtual ~CPUPerformanceCounter();
  CPUPerformanceCounter(IMGDFMetric *metric, Timer &timer,
                        LARGE_INTEGER frequency);

 protected:
  HRESULT DoBegin(std::map<std::string, std::string> &tags,
                  IMGDFPerformanceCounterScope **scope) final;

 private:
  LARGE_INTEGER _frequency;
};

/**
this class is used for timing
*/
class Timer : public ComBase<IMGDFTimer> {
 public:
  static HRESULT TryCreate(UINT32 frameSamples, ComObject<Timer> &timer);
  Timer(UINT32 maxSamples);
  virtual ~Timer(void);

  LARGE_INTEGER __stdcall GetCurrentTimeTicks() final;
  LARGE_INTEGER __stdcall GetTimerFrequency() final;
  double __stdcall ConvertDifferenceToSeconds(LARGE_INTEGER newTime,
                                              LARGE_INTEGER oldTime) final;

  HRESULT CreateCPUCounter(IMGDFMetric *metric,
                           IMGDFPerformanceCounter **counter);

  HRESULT CreateGPUCounter(IMGDFMetric *metric, ID3D12GraphicsCommandList *list,
                           IMGDFPerformanceCounter **counter);
  HRESULT InitFromDevice12(ID3D12Device10 *device, ID3D12CommandQueue *queue,
                           UINT slots);
  void BeginD3D12Frame(const MGDFFrameInfo &frame);
  HRESULT EndD3D12Frame();
  void BeforeDeviceReset();

  void GetCounterSnapshots(std::vector<CounterSnapshot> &snapshots) const;

  template <typename T>
  void RemoveCounter(T *counter) {
    if (!counter) return;
    std::lock_guard<std::mutex> lock(_mutex);
    RemoveInternal(counter);
  }

 private:
  HRESULT Init();

  void RemoveInternal(CPUPerformanceCounter *counter);

  std::unique_ptr<D3D12CounterManager> _d3d12Counters;
  LARGE_INTEGER _freq;

  UINT32 _maxSamples;

  mutable std::mutex _mutex;
  std::set<CPUPerformanceCounter *> _cpuCounters;
};  // namespace core

}  // namespace core
}  // namespace MGDF
