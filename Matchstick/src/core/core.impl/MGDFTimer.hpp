#pragma once

#include <MGDF/MGDF.h>
#include <d3d11.h>

#include <MGDF/ComObject.hpp>
#include <deque>
#include <list>
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
  virtual ~CounterBase();
  CounterBase(const char *name, UINT32 maxSamples, Timer &timer);

  HRESULT __stdcall GetName(char *name, UINT64 *length) final;

  double __stdcall GetAvgValue() final;

 protected:
  void AddSample(double sample);
  bool _started;
  std::string _name;
  Timer &_timer;

 private:
  mutable std::mutex _mutex;
  double _avg;

  UINT32 _maxSamples;
  std::deque<double> _samples;
};

class CPUPerformanceCounter : public CounterBase {
 public:
  virtual ~CPUPerformanceCounter();
  CPUPerformanceCounter(const char *name, UINT32 maxSamples, Timer &timer,
                        LARGE_INTEGER frequency);

  void __stdcall Begin() final;
  void __stdcall End() final;

 private:
  LARGE_INTEGER _start;
  LARGE_INTEGER _frequency;
};

class GPUPerformanceCounter : public CounterBase {
 public:
  virtual ~GPUPerformanceCounter();
  GPUPerformanceCounter(const char *name, UINT32 maxSamples, Timer &timer);

  void __stdcall Begin() final;
  void __stdcall End() final;

  void ForceEnd();
  void Init(const ComObject<ID3D11Device> &device,
            const ComObject<ID3D11DeviceContext> &context, UINT bufferSize);
  void Reset();
  void DataReady(ID3D11Query *disjoint, UINT64 frequency);
  void DataDisjoint(ID3D11Query *disjoint);
  void SetDisjointQuery(ID3D11Query *disjoint);

 private:
  std::unordered_map<ID3D11Query *,
                     std::pair<ComObject<ID3D11Query>, ComObject<ID3D11Query>>>
      _pendingQueries;
  std::stack<ComObject<ID3D11Query>> _beginQueries;
  std::stack<ComObject<ID3D11Query>> _endQueries;
  ID3D11Query *_currentDisjoint;
  ComObject<ID3D11DeviceContext> _context;
  bool _hasRun;
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

  HRESULT CreateCPUCounter(const char *name, IMGDFPerformanceCounter **counter);
  HRESULT CreateGPUCounter(const char *name, IMGDFPerformanceCounter **counter);

  void BeforeDeviceReset();
  void InitFromDevice(const ComObject<ID3D11Device> &device, UINT32 bufferSize);

  void Begin();
  void End();
  void GetCounterInformation(TextStream &outputStream) const;

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
  std::stack<ComObject<ID3D11Query>> _disjointQueries;
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
