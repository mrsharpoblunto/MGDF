#include "StdAfx.h"

#include <math.h>
#include <mmsystem.h>
#include <map>
#include "MGDFTimer.hpp"
#include "../common/MGDFLoggerImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

CounterBase::~CounterBase() { _timer.RemoveCounter(this); }

CounterBase::CounterBase(const char *name, UINT32 maxSamples, Timer &timer)
    : _name(name),
      _maxSamples(maxSamples),
      _references(1U),
      _avg(0),
      _timer(timer),
      _started(false) {
  _ASSERTE(name);
}

const char *CounterBase::GetName() const { return _name.c_str(); }

ULONG CounterBase::AddRef() { return ++_references; }

ULONG CounterBase::Release() {
  if (--_references == 0UL) {
    delete this;
    return 0UL;
  }
  return _references;
}

HRESULT CounterBase::QueryInterface(REFIID riid, void **ppvObject) {
  if (!ppvObject) return E_POINTER;
  if (riid == IID_IUnknown || riid == __uuidof(IPerformanceCounter)) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  }
  return E_NOINTERFACE;
}

void CounterBase::AddSample(double sample) {
  std::lock_guard<std::mutex> lock(_mutex);

  _samples.push_front(sample);
  if (_samples.size() >= _maxSamples) {
    _samples.pop_back();
  }
  _avg = 0;
  for (auto sample : _samples) {
    _avg += sample;
  }
  _avg /= _samples.size();
}

double CounterBase::GetAvgValue() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _avg;
}

/** ------------CPU counter ---------*/
CPUPerformanceCounter::~CPUPerformanceCounter() {}

CPUPerformanceCounter::CPUPerformanceCounter(const char *name,
                                             UINT32 maxSamples, Timer &timer,
                                             LARGE_INTEGER frequency)
    : CounterBase(name, maxSamples, timer), _frequency(frequency) {}

void CPUPerformanceCounter::Begin() {
  if (_started) {
    LOG("Call to CPU Timer Begin for Timer "
            << GetName() << " that has already started timing",
        LOG_ERROR);
  } else {
    _started = true;
    QueryPerformanceCounter(&_start);
  }
}

void CPUPerformanceCounter::End() {
  if (!_started) {
    LOG("Call to GPU Timer End without matching call to Begin on Timer "
            << GetName(),
        LOG_ERROR);
  } else {
    _started = false;
    LARGE_INTEGER newTime;
    QueryPerformanceCounter(&newTime);

    LONGLONG diff = newTime.QuadPart - min(_start.QuadPart, newTime.QuadPart);

    AddSample((double)diff / _frequency.QuadPart);
  }
}

/** ------------GPU counter ---------*/

GPUPerformanceCounter::~GPUPerformanceCounter() { Reset(); }

GPUPerformanceCounter::GPUPerformanceCounter(const char *name,
                                             UINT32 maxSamples, Timer &timer)
    : CounterBase(name, maxSamples, timer),
      _currentDisjoint(nullptr),
      _context(nullptr),
      _hasRun(false) {}

void GPUPerformanceCounter::SetDisjointQuery(ID3D11Query *disjoint) {
  _hasRun = false;
  _currentDisjoint = disjoint;
}

void GPUPerformanceCounter::Begin() {
  _ASSERTE(_context);

  if (_hasRun) {
    LOG("GPU Timer " << GetName() << " has already been used this frame",
        LOG_HIGH);
  } else if (_started) {
    LOG("Call to GPU Timer Begin for Timer "
            << GetName() << " that has already started timing",
        LOG_ERROR);
  } else if (_currentDisjoint) {
    if (!_beginQueries.empty()) {
      _ASSERTE(!_endQueries.empty());
      _ASSERTE(_pendingQueries.find(_currentDisjoint) == _pendingQueries.end());

      // map the begin and end queries to the current frames disjoint query
      _pendingQueries.insert(
          std::pair<ID3D11Query *, std::pair<ID3D11Query *, ID3D11Query *>>(
              _currentDisjoint, std::pair<ID3D11Query *, ID3D11Query *>(
                                    _beginQueries.top(), _endQueries.top())));

      _context->End(_beginQueries.top());
      _beginQueries.pop();
      _endQueries.pop();
      _started = true;
    } else {
      LOG("No available queries to begin GPU Timer " << GetName(), LOG_MEDIUM);
    }
  }
}

void GPUPerformanceCounter::End() {
  _ASSERTE(_context);

  if (_hasRun) return;

  if (_started) {
    _ASSERTE(_currentDisjoint);

    auto it = _pendingQueries.find(_currentDisjoint);
    _ASSERTE(it != _pendingQueries.end());
    _context->End(it->second.second);

    _started = false;
    _hasRun = true;
  } else if (_currentDisjoint) {
    LOG("Call to GPU Timer End without matching call to Begin on Timer "
            << GetName(),
        LOG_ERROR);
  }
}

void GPUPerformanceCounter::ForceEnd() {
  if (_started) {
    LOG("GPU Timer " << GetName()
                     << " that was Begun this frame was not Ended this frame - "
                        "forcing End",
        LOG_ERROR);
    End();
  }
}

void GPUPerformanceCounter::Init(ID3D11Device *device,
                                 ID3D11DeviceContext *context,
                                 UINT32 bufferSize) {
  _ASSERTE(device);
  _ASSERTE(context);
  _ASSERTE(bufferSize > 0);
  _context = context;

  // create a pool of queries to reuse
  for (UINT32 i = 0; i < bufferSize; ++i) {
    D3D11_QUERY_DESC desc;
    desc.Query = D3D11_QUERY_TIMESTAMP;
    desc.MiscFlags = 0;

    ID3D11Query *query;
    device->CreateQuery(&desc, &query);
    _beginQueries.push(query);
    device->CreateQuery(&desc, &query);
    _endQueries.push(query);
  }
}

void GPUPerformanceCounter::Reset() {
  while (!_beginQueries.empty()) {
    auto counter = _beginQueries.top();
    _beginQueries.pop();
    SAFE_RELEASE(counter);
  }
  while (!_endQueries.empty()) {
    auto counter = _endQueries.top();
    _endQueries.pop();
    SAFE_RELEASE(counter);
  }
  for (auto pending : _pendingQueries) {
    SAFE_RELEASE(pending.second.first);
    SAFE_RELEASE(pending.second.second);
  }
  _pendingQueries.clear();
  _currentDisjoint = nullptr;
}

void GPUPerformanceCounter::DataDisjoint(ID3D11Query *disjoint) {
  auto it = _pendingQueries.find(disjoint);
  if (it == _pendingQueries.end()) return;

  LOG("Ignoring GPU Timer sample from disjoint query", LOG_MEDIUM);
  _beginQueries.push(it->second.first);
  _endQueries.push(it->second.second);
  _pendingQueries.erase(it);
}

void GPUPerformanceCounter::DataReady(ID3D11Query *disjoint, UINT64 frequency) {
  auto it = _pendingQueries.find(disjoint);
  if (it == _pendingQueries.end()) return;

  bool success = true;

  UINT64 timeStampBegin;
  if (_context->GetData(it->second.first, &timeStampBegin, sizeof(UINT64), 0) !=
      S_OK) {
    LOG("Failed to bet Begin Data for GPU Timer " << GetName()
                                                  << "- Ignoring sample",
        LOG_ERROR);
    success = false;
  }

  UINT64 timeStampEnd;
  if (success && _context->GetData(it->second.second, &timeStampEnd,
                                   sizeof(UINT64), 0) != S_OK) {
    LOG("Failed to bet End Data for GPU Timer " << GetName()
                                                << "- Ignoring sample",
        LOG_ERROR);
    success = false;
  }

  if (success) {
    UINT64 diff = timeStampEnd - min(timeStampBegin, timeStampEnd);
    double value = ((double)diff / frequency);
    AddSample(value);
  }

  _beginQueries.push(it->second.first);
  _endQueries.push(it->second.second);
  _pendingQueries.erase(it);
}

/** -------------- Timer ------------*/

MGDFError Timer::TryCreate(UINT32 maxSamples, Timer **timer) {
  *timer = new Timer(maxSamples);
  MGDFError error = (*timer)->Init();
  if (MGDF_OK != error) {
    delete *timer;
    *timer = nullptr;
  }
  return error;
}

Timer::Timer(UINT32 maxSamples)
    : _device(nullptr),
      _maxSamples(maxSamples),
      _bufferSize(0),
      _context(nullptr),
      _gpuTimersSupported(true) {
  _ASSERTE(maxSamples > 0);
}

MGDFError Timer::Init() {
  // exit if the  does not support a high performance timer
  if (!QueryPerformanceFrequency(&_freq)) {
    LOG("High performance timer unsupported", LOG_ERROR);
    return MGDF_ERR_CPU_TIMER_UNSUPPORTED;
  }
  return MGDF_OK;
}

Timer::~Timer(void) {
  while (_cpuCounters.size() > 0) {
    LOG("CPUTimer '" << _cpuCounters.back()->GetName() << "' still has "
                     << _cpuCounters.back()->RefCount() << " live references",
        LOG_ERROR);
    delete _cpuCounters.back();
    _cpuCounters.pop_back();
  }
  while (_gpuCounters.size() > 0) {
    LOG("GPUTimer '" << _gpuCounters.back()->GetName() << "' still has "
                     << _gpuCounters.back()->RefCount() << " live references",
        LOG_ERROR);
    delete _gpuCounters.back();
    _gpuCounters.pop_back();
  }

  ResetGPUTimers();
}

void Timer::ResetGPUTimers() {
  LOG("Cleaning up GPU Timers...", LOG_LOW);
  while (!_disjointQueries.empty()) {
    auto query = _disjointQueries.top();
    _disjointQueries.pop();
    SAFE_RELEASE(query);
  }
  for (auto counter : _pendingQueries) {
    SAFE_RELEASE(counter);
  }
  _pendingQueries.clear();
  SAFE_RELEASE(_context);

  for (auto gpuCounter : _gpuCounters) {
    gpuCounter->Reset();
  }
}

void Timer::InitFromDevice(ID3D11Device *device, UINT32 bufferSize) {
  _ASSERTE(device);
  _ASSERTE(bufferSize > 0);

  if (_device) {
    // if a device is reset, then we need to
    // clean up any old counters and contexts
    ResetGPUTimers();
  }

  _bufferSize = bufferSize;
  _device = device;
  device->GetImmediateContext(&_context);

  LOG("Initializing GPU Timers...", LOG_LOW);

  // create a pool of queries to reuse
  for (UINT32 i = 0; i < bufferSize; ++i) {
    D3D11_QUERY_DESC desc;
    desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    desc.MiscFlags = 0;

    ID3D11Query *query;
    _device->CreateQuery(&desc, &query);
    if (!query) {
      LOG("GPU timing queries unsupported", LOG_ERROR);
      _gpuTimersSupported = false;
      break;
    }
    _disjointQueries.push(query);
  }

  for (auto gpuCounter : _gpuCounters) {
    gpuCounter->Init(_device, _context, bufferSize);
  }
}

LARGE_INTEGER Timer::GetCurrentTimeTicks() const {
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

LARGE_INTEGER Timer::GetTimerFrequency() const { return _freq; }

double Timer::ConvertDifferenceToSeconds(LARGE_INTEGER newTime,
                                         LARGE_INTEGER oldTime) const {
  LONGLONG diff = max(newTime.QuadPart, oldTime.QuadPart) -
                  min(newTime.QuadPart, oldTime.QuadPart);
  return max((double)diff / _freq.QuadPart, 0);
}

MGDFError Timer::CreateCPUCounter(const char *name,
                                  IPerformanceCounter **counter) {
  if (!name) return MGDF_ERR_INVALID_TIMER_NAME;

  CPUPerformanceCounter *c =
      new CPUPerformanceCounter(name, _maxSamples, *this, _freq);

  std::lock_guard<std::mutex> lock(_mutex);
  _cpuCounters.push_back(c);
  *counter = c;
  return MGDF_OK;
}

MGDFError Timer::CreateGPUCounter(const char *name,
                                  IPerformanceCounter **counter) {
  if (!_gpuTimersSupported) return MGDF_ERR_GPU_TIMER_UNSUPPORTED;
  if (!name) return MGDF_ERR_INVALID_TIMER_NAME;
  GPUPerformanceCounter *c =
      new GPUPerformanceCounter(name, _maxSamples, *this);

  if (_device) {
    c->Init(_device, _context, _bufferSize);
  }

  std::lock_guard<std::mutex> lock(_mutex);
  _gpuCounters.push_back(c);
  *counter = c;
  return MGDF_OK;
}

void Timer::RemoveCounter(IPerformanceCounter *counter) {
  if (!counter) return;

  std::lock_guard<std::mutex> lock(_mutex);
  for (auto iter = _cpuCounters.begin(); iter != _cpuCounters.end(); ++iter) {
    if (*iter == counter) {
      _cpuCounters.erase(iter);
      return;
    }
  }
  for (auto iter = _gpuCounters.begin(); iter != _gpuCounters.end(); ++iter) {
    if (*iter == counter) {
      _gpuCounters.erase(iter);
      return;
    }
  }

  _ASSERTE(0);
}

void Timer::Begin() {
  if (_gpuTimersSupported) {
    if (!_pendingQueries.empty()) {
      // see if the oldest query is ready yet
      auto query = _pendingQueries.back();
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjoint;

      // if it is, then notify the gpu timers waiting for this query
      if (_context->GetData(query, &disjoint,
                            sizeof(D3D11_QUERY_DATA_TIMESTAMP_DISJOINT),
                            0) == S_OK) {
        std::lock_guard<std::mutex> lock(_mutex);
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
        _disjointQueries.push(query);
      }
    }

    std::lock_guard<std::mutex> lock(_mutex);

    // if theres a free disjoint query available in the pool, use it
    // and notify all timers
    if (!_disjointQueries.empty()) {
      _currentQuery = _disjointQueries.top();
      _disjointQueries.pop();
      _pendingQueries.push_front(_currentQuery);

      _context->Begin(_currentQuery);
      for (auto counter : _gpuCounters) {
        counter->SetDisjointQuery(_currentQuery);
      }
    } else {
      LOG("No available queries to record GPU timer disjoint", LOG_MEDIUM);
      _currentQuery = nullptr;
      for (auto counter : _gpuCounters) {
        counter->SetDisjointQuery(nullptr);
      }
    }
  }
}

void Timer::End() {
  // force any open counters to end if they haven't already
  std::lock_guard<std::mutex> lock(_mutex);
  for (auto counter : _gpuCounters) {
    counter->ForceEnd();
  }
  if (_gpuTimersSupported && _currentQuery) _context->End(_currentQuery);
}

void Timer::GetCounterInformation(TextStream &outputStream) const {
  std::lock_guard<std::mutex> lock(_mutex);

  if (_gpuCounters.size() > 0) {
    outputStream << TextStyle::Weight(DWRITE_FONT_WEIGHT_BOLD) << "\r\n\r\nGPU"
                 << TextStyle::Pop();
    KeyValueHeatMap<GPUPerformanceCounter *, double>(
        _gpuCounters,
        [](const auto counter, auto &out) {
          out.first = counter->GetName();
          out.second = counter->GetAvgValue();
        },
        outputStream);
  }

  if (_cpuCounters.size() > 0) {
    outputStream << TextStyle::Weight(DWRITE_FONT_WEIGHT_BOLD) << "\r\n\r\nCPU"
                 << TextStyle::Pop();
    KeyValueHeatMap<CPUPerformanceCounter *, double>(
        _cpuCounters,
        [](const auto counter, auto &out) {
          out.first = counter->GetName();
          out.second = counter->GetAvgValue();
        },
        outputStream);
  }
}

}  // namespace core
}  // namespace MGDF