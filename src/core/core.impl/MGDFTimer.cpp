#include "StdAfx.h"

#include "MGDFTimer.hpp"

#include <map>

#include "../common/MGDFLoggerImpl.hpp"
#include "MGDFD3D12Timer.hpp"
#include "MGDFMetrics.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

CounterBase::~CounterBase() {}

CounterBase::CounterBase(IMGDFMetric *metric, Timer &timer)
    : _timer(timer),
      _average(0U),
      _metric(MakeComFromPtr<IMGDFMetric>(metric)) {}

void CounterBase::AddSample(double sample) {
  std::lock_guard<std::mutex> lock(_mutex);

  _samples.push_front(sample);
  if (_samples.size() >= MaxSamples) {
    _samples.pop_back();
  }
  _average = 0;
  for (auto s : _samples) {
    _average += s;
  }
  _average /= _samples.size();
}

double CounterBase::GetAverageValue() {
  std::lock_guard<std::mutex> lock(_mutex);
  return _average;
}

void CounterBase::Snapshot(CounterSnapshot &snapshot, bool gpu) {
  std::lock_guard<std::mutex> lock(_mutex);
  // every metric the host hands out is a MetricBase
  snapshot.Name = static_cast<MetricBase *>(_metric.Get())->GetName();
  snapshot.GPU = gpu;
  snapshot.Average = _average;
  snapshot.Samples.assign(_samples.begin(), _samples.end());
}

void CounterBase::GetMetric(IMGDFMetric **metric) { _metric.AddRawRef(metric); }

HRESULT CounterBase::Begin(const MGDFTags *tags,
                           IMGDFPerformanceCounterScope **scope) {
  std::map<std::string, std::string> tagMap;
  if (tags) {
    for (UINT64 i = 0; i < tags->Count; ++i) {
      tagMap.emplace(std::make_pair(std::string(tags->Names[i]),
                                    std::string(tags->Values[i])));
    }
  }
  return DoBegin(tagMap, scope);
}

/** ------------CPU counter ---------*/
CPUPerformanceCounterScope::CPUPerformanceCounterScope(
    CPUPerformanceCounter *counter, std::map<std::string, std::string> &tags)
    : _counter(counter), _tags(std::move(tags)) {
  QueryPerformanceCounter(&_start);
}
CPUPerformanceCounterScope::~CPUPerformanceCounterScope() {
  LARGE_INTEGER newTime;
  QueryPerformanceCounter(&newTime);

  const LONGLONG diff =
      newTime.QuadPart - min(_start.QuadPart, newTime.QuadPart);

  std::vector<const char *> tagNames;
  std::vector<const char *> tagValues;
  tagNames.reserve(_tags.size());
  tagValues.reserve(_tags.size());
  for (auto &it : _tags) {
    tagNames.emplace_back(it.first.c_str());
    tagValues.emplace_back(it.second.c_str());
  }
  const auto value = (double)diff / _counter->_frequency.QuadPart;
  _counter->AddSample(value);
  const MGDFTags tags{.Names = tagNames.data(),
                      .Values = tagValues.data(),
                      .Count = tagNames.size()};
  _counter->_metric->Record(value, &tags);
}

CPUPerformanceCounter::~CPUPerformanceCounter() { _timer.RemoveCounter(this); }

CPUPerformanceCounter::CPUPerformanceCounter(IMGDFMetric *metric, Timer &timer,
                                             LARGE_INTEGER frequency)
    : CounterBase(metric, timer), _frequency(frequency) {}

HRESULT CPUPerformanceCounter::DoBegin(std::map<std::string, std::string> &tags,
                                       IMGDFPerformanceCounterScope **scope) {
  auto newScope = MakeCom<CPUPerformanceCounterScope>(this, tags);
  newScope.AddRawRef(scope);
  return S_OK;
}

HRESULT Timer::TryCreate(UINT32 maxSamples, ComObject<Timer> &timer) {
  timer = MakeCom<Timer>(maxSamples);
  const HRESULT result = timer->Init();
  if (FAILED(result)) {
    timer.Clear();
  }
  return result;
}

Timer::Timer(UINT32 maxSamples)
    : _d3d12Counters(std::make_unique<D3D12CounterManager>(*this)),
      _maxSamples(maxSamples) {
  _ASSERTE(maxSamples > 0);
  ZeroMemory(&_freq, sizeof(LARGE_INTEGER));
}

HRESULT Timer::Init() {
  // exit if the  does not support a high performance timer
  if (!QueryPerformanceFrequency(&_freq)) {
    LOG("High performance timer unsupported", MGDF_LOG_ERROR);
    return E_FAIL;
  }
  return S_OK;
}

Timer::~Timer(void) {
  if (_cpuCounters.size()) {
    LOG("CPUTimer still has " << _cpuCounters.size() << " live references",
        MGDF_LOG_ERROR);
  }
}

void Timer::BeforeDeviceReset() { _d3d12Counters->Reset(); }

LARGE_INTEGER Timer::GetCurrentTimeTicks() {
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

LARGE_INTEGER Timer::GetTimerFrequency() { return _freq; }

double Timer::ConvertDifferenceToSeconds(LARGE_INTEGER newTime,
                                         LARGE_INTEGER oldTime) {
  const LONGLONG diff = max(newTime.QuadPart, oldTime.QuadPart) -
                        min(newTime.QuadPart, oldTime.QuadPart);
  return max((double)diff / _freq.QuadPart, 0);
}

void Timer::GetCounterSnapshots(std::vector<CounterSnapshot> &snapshots) const {
  std::lock_guard<std::mutex> lock(_mutex);
  _d3d12Counters->Snapshots(snapshots);
  snapshots.reserve(snapshots.size() + _cpuCounters.size());
  for (auto *counter : _cpuCounters) {
    counter->Snapshot(snapshots.emplace_back(), false);
  }
}

HRESULT Timer::CreateCPUCounter(IMGDFMetric *metric,
                                IMGDFPerformanceCounter **counter) {
  if (!metric) return E_FAIL;
  auto c = MakeCom<CPUPerformanceCounter>(metric, *this, _freq);

  std::lock_guard<std::mutex> lock(_mutex);
  _cpuCounters.insert(c);
  c.AddRawRef(counter);
  return S_OK;
}

void Timer::RemoveInternal(CPUPerformanceCounter *counter) {
  _cpuCounters.erase(counter);
}

HRESULT Timer::InitFromDevice12(ID3D12Device10 *device,
                                ID3D12CommandQueue *queue, UINT slots) {
  return _d3d12Counters->Init(device, queue, slots);
}
void Timer::BeginD3D12Frame(const MGDFFrameInfo &frame) {
  _d3d12Counters->BeginFrame(frame);
}
HRESULT Timer::EndD3D12Frame() { return _d3d12Counters->EndFrame(); }
HRESULT Timer::CreateGPUCounter(IMGDFMetric *metric,
                                ID3D12GraphicsCommandList *list,
                                IMGDFPerformanceCounter **counter) {
  return _d3d12Counters->Create(metric, list, counter);
}

}  // namespace core
}  // namespace MGDF