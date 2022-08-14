#pragma once

#include <MGDF/MGDF.h>
#include <d3d11.h>

#include <MGDF/ComObject.hpp>
#include <deque>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <stack>
#include <unordered_map>
#include <vector>

#include "MGDFTextStream.hpp"

namespace MGDF {
namespace core {

class Timer;

class CounterBase : public ComBase<IMGDFPerformanceCounter> {
 public:
  static constexpr const size_t MaxSamples = 5;

  virtual ~CounterBase();
  CounterBase(IMGDFMetric *metric, Timer &timer);

  void __stdcall GetMetric(IMGDFMetric **metric) final;
  HRESULT __stdcall Begin(IMGDFPerformanceCounterScope **scope) final {
    return BeginTagged(nullptr, nullptr, 0, scope);
  }
  HRESULT __stdcall BeginTagged(const small **tags, const small **tagValues,
                                UINT64 tagCount,
                                IMGDFPerformanceCounterScope **scope) final;

  double __stdcall GetAverageValue() final;

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

class GPUPerformanceCounter;

class GPUPerformanceCounterScope
    : public ComBase<IMGDFPerformanceCounterScope> {
 public:
  GPUPerformanceCounterScope(ComObject<ID3D11DeviceContext> &context,
                             ComObject<ID3D11Query> &endQuery)
      : _context(context), _endQuery(endQuery) {}
  virtual ~GPUPerformanceCounterScope();

 private:
  ComObject<ID3D11DeviceContext> _context;
  ComObject<ID3D11Query> _endQuery;
};

class GPUPerformanceCounter : public CounterBase {
 public:
  virtual ~GPUPerformanceCounter();
  GPUPerformanceCounter(IMGDFMetric *metric, Timer &timer);

  void Init(const ComObject<ID3D11Device> &device,
            const ComObject<ID3D11DeviceContext> &context);
  void Reset();
  void DataReady(ID3D11Query *disjoint, UINT64 frequency);
  void DataDisjoint(ID3D11Query *disjoint);
  void SetDisjointQuery(ID3D11Query *disjoint);

 protected:
  HRESULT DoBegin(std::map<std::string, std::string> &tags,
                  IMGDFPerformanceCounterScope **scope) final;

 private:
  struct ScopeMapping {
    ComObject<ID3D11Query> Begin;
    ComObject<ID3D11Query> End;
    std::map<std::string, std::string> Tags;
  };

  std::unordered_map<ID3D11Query *, std::vector<ScopeMapping>> _pendingQueries;
  std::stack<ComObject<ID3D11Query>> _beginQueries;
  std::stack<ComObject<ID3D11Query>> _endQueries;
  ID3D11Query *_currentDisjoint;
  ComObject<ID3D11Device> _device;
  ComObject<ID3D11DeviceContext> _context;
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
  HRESULT CreateGPUCounter(IMGDFMetric *metric,
                           IMGDFPerformanceCounter **counter);

  void BeforeDeviceReset();
  void InitFromDevice(const ComObject<ID3D11Device> &device, UINT32 bufferSize);

  void Begin();
  void End();

  template <typename T>
  void RemoveCounter(T *counter) {
    if (!counter) return;
    std::lock_guard<std::mutex> lock(_mutex);
    RemoveInternal(counter);
  }

 private:
  HRESULT Init();
  void ResetGPUTimers();

  template <typename T>
  void RemoveInternal(T *counter) {
    static_assert(false, "Not allowed");
  }

  ComObject<ID3D11Device> _device;
  ComObject<ID3D11DeviceContext> _context;
  LARGE_INTEGER _freq;

  std::deque<ComObject<ID3D11Query>> _pendingQueries;
  std::vector<ComObject<ID3D11Query>> _disjointQueries;
  ComObject<ID3D11Query> _currentQuery;

  UINT32 _bufferSize;
  UINT32 _maxSamples;
  bool _gpuTimersSupported;

  mutable std::mutex _mutex;
  std::set<CPUPerformanceCounter *> _cpuCounters;
  std::set<GPUPerformanceCounter *> _gpuCounters;
};  // namespace core

template <>
void Timer::RemoveInternal(GPUPerformanceCounter *counter);

template <>
void Timer::RemoveInternal(CPUPerformanceCounter *counter);
}  // namespace core
}  // namespace MGDF
