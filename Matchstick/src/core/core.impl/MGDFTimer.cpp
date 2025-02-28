#include "StdAfx.h"

#include "MGDFTimer.hpp"

#include <map>

#include "../common/MGDFLoggerImpl.hpp"

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

/** ------------GPU counter ---------*/

GPUPerformanceCounter::~GPUPerformanceCounter() {
  _timer.RemoveCounter(this);
  Reset();
}

GPUPerformanceCounter::GPUPerformanceCounter(IMGDFMetric *metric, Timer &timer)
    : CounterBase(metric, timer),
      _currentDisjoint(nullptr),
      _context(nullptr) {}

void GPUPerformanceCounter::SetDisjointQuery(ID3D11Query *disjoint) {
  _currentDisjoint = disjoint;
}

HRESULT GPUPerformanceCounter::DoBegin(std::map<std::string, std::string> &tags,
                                       IMGDFPerformanceCounterScope **scope) {
  if (_context && _currentDisjoint) {
    if (_beginQueries.empty()) {
      D3D11_QUERY_DESC desc;
      desc.Query = D3D11_QUERY_TIMESTAMP;
      desc.MiscFlags = 0;

      if (!(SUCCEEDED(_device->CreateQuery(&desc,
                                           _beginQueries.emplace().Assign())) &&
            SUCCEEDED(
                _device->CreateQuery(&desc, _endQueries.emplace().Assign())))) {
        Reset();
        LOG("Failed to create GPU timing queries", MGDF_LOG_ERROR);
        return E_FAIL;
      }
    }
    _ASSERTE(!_endQueries.empty());

    // map the begin and end queries to the current frames disjoint query
    auto currentDisjointPending = _pendingQueries.find(_currentDisjoint);
    if (currentDisjointPending == _pendingQueries.end()) {
      currentDisjointPending =
          _pendingQueries
              .emplace(
                  std::make_pair(_currentDisjoint, std::vector<ScopeMapping>()))
              .first;
    }
    auto scopeMapping = currentDisjointPending->second.emplace_back(
        ScopeMapping{.Begin = _beginQueries.top(),
                     .End = _endQueries.top(),
                     .Tags = std::move(tags)});
    _context->End(scopeMapping.Begin);
    _beginQueries.pop();
    _endQueries.pop();

    auto newScope =
        MakeCom<GPUPerformanceCounterScope>(_context, scopeMapping.End);
    newScope.AddRawRef(scope);
    return S_OK;
  }
  return E_FAIL;
}

GPUPerformanceCounterScope::~GPUPerformanceCounterScope() {
  if (_context) {
    _ASSERTE(_endQuery);
    _context->End(_endQuery);
  }
}

HRESULT GPUPerformanceCounter::Init(const ComObject<ID3D11Device> &device,
                                    ID3D11DeviceContext *context) {
  _ASSERTE(device);
  _ASSERTE(context);

  _device = device;
  if (context->GetType() != D3D11_DEVICE_CONTEXT_IMMEDIATE) {
    LOG("Unable to Initialize GPU Timer using a deferred context",
        MGDF_LOG_ERROR);
    _context.Clear();
    return E_FAIL;
  } else {
    _context = ComObject<ID3D11DeviceContext>(context, true);
    return S_OK;
  }
}

void GPUPerformanceCounter::Reset() {
  _context.Clear();
  while (!_beginQueries.empty()) {
    _beginQueries.pop();
  }
  while (!_endQueries.empty()) {
    _endQueries.pop();
  }
  _pendingQueries.clear();
  _currentDisjoint = nullptr;
}

void GPUPerformanceCounter::DataDisjoint(ID3D11Query *disjoint) {
  const auto it = _pendingQueries.find(disjoint);
  if (it == _pendingQueries.end()) return;

  LOG("Ignoring GPU Timer sample from disjoint query", MGDF_LOG_MEDIUM);
  for (auto &jt : it->second) {
    _beginQueries.push(jt.Begin);
    _endQueries.push(jt.End);
  }
  _pendingQueries.erase(it);
}

void GPUPerformanceCounter::DataReady(ID3D11Query *disjoint, UINT64 frequency) {
  const auto it = _pendingQueries.find(disjoint);
  if (it == _pendingQueries.end()) return;

  for (auto &jt : it->second) {
    UINT64 timeStampBegin = 0;
    if (!_context || _context->GetData(jt.Begin, &timeStampBegin,
                                       sizeof(UINT64), 0) != S_OK) {
      LOG("Failed to get Begin Data for GPU Timer - Ignoring sample",
          MGDF_LOG_ERROR);
    } else {
      UINT64 timeStampEnd = 0;
      if (!_context ||
          _context->GetData(jt.End, &timeStampEnd, sizeof(UINT64), 0) != S_OK) {
        LOG("Failed to get End Data for GPU Timer - Ignoring sample",
            MGDF_LOG_ERROR);
      } else {
        const UINT64 diff = timeStampEnd - min(timeStampBegin, timeStampEnd);
        const double value = ((double)diff / frequency);
        std::vector<const char *> tagNames;
        std::vector<const char *> tagValues;
        tagNames.reserve(jt.Tags.size());
        tagValues.reserve(jt.Tags.size());
        for (auto &t : jt.Tags) {
          tagNames.emplace_back(t.first.c_str());
          tagValues.emplace_back(t.second.c_str());
        }
        AddSample(value);
        const MGDFTags tags{.Names = tagNames.data(),
                            .Values = tagValues.data(),
                            .Count = tagNames.size()};
        _metric->Record(value, &tags);
      }
    }

    _beginQueries.push(jt.Begin);
    _endQueries.push(jt.End);
  }
  _pendingQueries.erase(it);
}

/** -------------- Timer ------------*/

HRESULT Timer::TryCreate(UINT32 maxSamples, ComObject<Timer> &timer) {
  timer = MakeCom<Timer>(maxSamples);
  const HRESULT result = timer->Init();
  if (FAILED(result)) {
    timer.Clear();
  }
  return result;
}

Timer::Timer(UINT32 maxSamples)
    : _device(nullptr),
      _maxSamples(maxSamples),
      _bufferSize(0),
      _context(nullptr),
      _gpuTimersSupported(true) {
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
  if (_gpuCounters.size()) {
    LOG("GPUTimer still has " << _gpuCounters.size() << " live references",
        MGDF_LOG_ERROR);
  }

  ResetGPUTimers();
}

void Timer::ResetGPUTimers() {
  LOG("Cleaning up GPU Timers...", MGDF_LOG_LOW);
  while (!_disjointQueries.empty()) {
    _disjointQueries.pop_back();
  }
  _pendingQueries.clear();

  for (auto gpuCounter : _gpuCounters) {
    gpuCounter->Reset();
  }
}

void Timer::BeforeDeviceReset() {
  _device.Clear();
  _context.Clear();
  ResetGPUTimers();
}

void Timer::InitFromDevice(const ComObject<ID3D11Device> &device,
                           UINT32 bufferSize) {
  _ASSERTE(device);
  _ASSERTE(bufferSize > 0);

  _bufferSize = bufferSize;
  _device = device;
  device->GetImmediateContext(_context.Assign());

  LOG("Initializing GPU Timers...", MGDF_LOG_LOW);

  // create a pool of queries to reuse
  for (UINT32 i = 0; i < bufferSize; ++i) {
    D3D11_QUERY_DESC desc;
    desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    desc.MiscFlags = 0;

    ComObject<ID3D11Query> query;
    if (FAILED(_device->CreateQuery(&desc, query.Assign()))) {
      LOG("GPU timing queries unsupported", MGDF_LOG_ERROR);
      _gpuTimersSupported = false;
      break;
    }
    _ASSERTE(query);
    _disjointQueries.push_back(query);
  }

  for (auto gpuCounter : _gpuCounters) {
    gpuCounter->Init(_device, _context);
  }
}

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

HRESULT Timer::CreateCPUCounter(IMGDFMetric *metric,
                                IMGDFPerformanceCounter **counter) {
  if (!metric) return E_FAIL;
  auto c = MakeCom<CPUPerformanceCounter>(metric, *this, _freq);

  std::lock_guard<std::mutex> lock(_mutex);
  _cpuCounters.insert(c);
  c.AddRawRef(counter);
  return S_OK;
}

HRESULT Timer::CreateGPUCounter(IMGDFMetric *metric,
                                ID3D11DeviceContext *context,
                                IMGDFPerformanceCounter **counter) {
  if (!_gpuTimersSupported || !metric) return E_FAIL;
  auto c = MakeCom<GPUPerformanceCounter>(metric, *this);

  if (!_device || c->Init(_device, context) == E_FAIL) {
    return E_FAIL;
  }

  std::lock_guard<std::mutex> lock(_mutex);
  _gpuCounters.insert(c);
  c.AddRawRef(counter);
  return S_OK;
}

void Timer::RemoveInternal(CPUPerformanceCounter *counter) {
  _cpuCounters.erase(counter);
}

void Timer::RemoveInternal(GPUPerformanceCounter *counter) {
  _gpuCounters.erase(counter);
}

void Timer::Begin() {
  if (_gpuTimersSupported) {
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_pendingQueries.empty()) {
      // see if the oldest query is ready yet
      const auto query = _pendingQueries.back();
      _ASSERTE(query);
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint = {};

      // if it is, then notify the gpu timers waiting for this query
      if (_context->GetData(query, &disjoint,
                            sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT),
                            0) == S_OK) {
        for (auto counter : _gpuCounters) {
          if (!disjoint.Disjoint) {
            // data is ready, let the timer know
            counter->DataReady(query, disjoint.Frequency);
          } else {
            // data is invalid and should be disregarded
            counter->DataDisjoint(query);
          }
        }

        // remove the query from active checking and push
        // it back into the query pool
        _pendingQueries.pop_back();
        _ASSERTE(query);
        _disjointQueries.push_back(query);
      }
    }

    // if theres a free disjoint query available in the pool, use it
    // and notify all timers
    if (!_disjointQueries.empty()) {
      const auto query = _currentQuery = _disjointQueries.back();
      _ASSERTE(query && _currentQuery);
      _pendingQueries.push_front(query);
      _disjointQueries.pop_back();

      _context->Begin(query);
      for (auto counter : _gpuCounters) {
        counter->SetDisjointQuery(query);
      }
    } else {
      LOG("No available queries to record GPU timer disjoint", MGDF_LOG_MEDIUM);
      _currentQuery.Clear();
      for (auto counter : _gpuCounters) {
        counter->SetDisjointQuery(nullptr);
      }
    }
  }
}

void Timer::End() {
  // force any open counters to end if they haven't already
  std::lock_guard<std::mutex> lock(_mutex);
  if (_gpuTimersSupported && _currentQuery) {
    _context->End(_currentQuery);
    _currentQuery.Clear();
  }
}

}  // namespace core
}  // namespace MGDF