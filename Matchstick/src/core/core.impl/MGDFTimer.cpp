#include "StdAfx.h"

#include "MGDFTimer.hpp"

#include <math.h>
#include <mmsystem.h>

#include <map>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFStringImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

CounterBase::~CounterBase() {}

CounterBase::CounterBase(const char *name, UINT32 maxSamples, Timer &timer)
    : _name(name),
      _maxSamples(maxSamples),
      _avg(0),
      _timer(timer),
      _started(false) {
  _ASSERTE(name);
}

HRESULT CounterBase::GetName(char *name, size_t *length) const {
  COPY_STR(_name, name, length);
}

void CounterBase::AddSample(double sample) {
  std::lock_guard<std::mutex> lock(_mutex);

  _samples.push_front(sample);
  if (_samples.size() >= _maxSamples) {
    _samples.pop_back();
  }
  _avg = 0;
  for (auto s : _samples) {
    _avg += s;
  }
  _avg /= _samples.size();
}

double CounterBase::GetAvgValue() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _avg;
}

/** ------------CPU counter ---------*/
CPUPerformanceCounter::~CPUPerformanceCounter() { _timer.RemoveCounter(this); }

CPUPerformanceCounter::CPUPerformanceCounter(const char *name,
                                             UINT32 maxSamples, Timer &timer,
                                             LARGE_INTEGER frequency)
    : CounterBase(name, maxSamples, timer), _frequency(frequency) {
  ZeroMemory(&_start, sizeof(LARGE_INTEGER));
}

void CPUPerformanceCounter::Begin() {
  if (_started) {
    LOG("Call to CPU Timer Begin for Timer "
            << _name.c_str() << " that has already started timing",
        LOG_ERROR);
  } else {
    _started = true;
    QueryPerformanceCounter(&_start);
  }
}

void CPUPerformanceCounter::End() {
  if (!_started) {
    LOG("Call to GPU Timer End without matching call to Begin on Timer "
            << _name.c_str(),
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

GPUPerformanceCounter::~GPUPerformanceCounter() {
  Reset();
  _timer.RemoveCounter(this);
}

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
    LOG("GPU Timer " << _name.c_str() << " has already been used this frame",
        LOG_HIGH);
  } else if (_started) {
    LOG("Call to GPU Timer Begin for Timer "
            << _name.c_str() << " that has already started timing",
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
      LOG("No available queries to begin GPU Timer " << _name.c_str(),
          LOG_MEDIUM);
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
            << _name.c_str(),
        LOG_ERROR);
  }
}

void GPUPerformanceCounter::ForceEnd() {
  if (_started) {
    LOG("GPU Timer " << _name.c_str()
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

  UINT64 timeStampBegin;
  if (_context->GetData(it->second.first, &timeStampBegin, sizeof(UINT64), 0) !=
      S_OK) {
    LOG("Failed to get Begin Data for GPU Timer " << _name.c_str()
                                                  << "- Ignoring sample",
        LOG_ERROR);
  } else {
    UINT64 timeStampEnd;
    if (_context->GetData(it->second.second, &timeStampEnd, sizeof(UINT64),
                          0) != S_OK) {
      LOG("Failed to bet End Data for GPU Timer " << _name.c_str()
                                                  << "- Ignoring sample",
          LOG_ERROR);
    } else {
      UINT64 diff = timeStampEnd - min(timeStampBegin, timeStampEnd);
      double value = ((double)diff / frequency);
      AddSample(value);
    }
  }

  _beginQueries.push(it->second.first);
  _endQueries.push(it->second.second);
  _pendingQueries.erase(it);
}

/** -------------- Timer ------------*/

HRESULT Timer::TryCreate(UINT32 maxSamples, ComObject<Timer> &timer) {
  ComObject<Timer> t(new Timer(maxSamples));
  if (FAILED(t->Init())) {
    timer = nullptr;
  } else {
    timer = t;
  }
  return S_OK;
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
    LOG("High performance timer unsupported", LOG_ERROR);
    return E_FAIL;
  }
  return S_OK;
}

Timer::~Timer(void) {
  for (auto &counter : _cpuCounters) {
    LOG("CPUTimer '" << STR(counter, GetName) << "' still has live references",
        LOG_ERROR);
  }
  for (auto &counter : _gpuCounters) {
    LOG("GPUTimer '" << STR(counter, GetName) << "' still has live references",
        LOG_ERROR);
  }

  ResetGPUTimers();
}

void Timer::ResetGPUTimers() {
  LOG("Cleaning up GPU Timers...", LOG_LOW);
  while (!_disjointQueries.empty()) {
    _disjointQueries.pop();
  }
  _pendingQueries.clear();

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
  device->GetImmediateContext(_context.Assign());

  LOG("Initializing GPU Timers...", LOG_LOW);

  // create a pool of queries to reuse
  for (UINT32 i = 0; i < bufferSize; ++i) {
    D3D11_QUERY_DESC desc;
    desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    desc.MiscFlags = 0;

    ComObject<ID3D11Query> query;
    _device->CreateQuery(&desc, query.Assign());
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

HRESULT Timer::CreateCPUCounter(const char *name,
                                IPerformanceCounter **counter) {
  if (!name) return E_FAIL;

  ComObject<CPUPerformanceCounter> c(
      new CPUPerformanceCounter(name, _maxSamples, *this, _freq));

  std::lock_guard<std::mutex> lock(_mutex);
  _cpuCounters.insert(c);
  c.AddRawRef(counter);
  return S_OK;
}

HRESULT Timer::CreateGPUCounter(const char *name,
                                IPerformanceCounter **counter) {
  if (!_gpuTimersSupported || !name) return E_FAIL;
  ComObject<GPUPerformanceCounter> c(
      new GPUPerformanceCounter(name, _maxSamples, *this));

  if (_device) {
    c->Init(_device, _context, _bufferSize);
  }

  std::lock_guard<std::mutex> lock(_mutex);
  _gpuCounters.insert(c);
  c.AddRawRef(counter);
  return S_OK;
}

template <>
void Timer::RemoveInternal(CPUPerformanceCounter *counter) {
  _cpuCounters.erase(counter);
}

template <>
void Timer::RemoveInternal(GPUPerformanceCounter *counter) {
  _gpuCounters.erase(counter);
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
          out.first = STR(counter, GetName);
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
          out.first = STR(counter, GetName);
          out.second = counter->GetAvgValue();
        },
        outputStream);
  }
}

}  // namespace core
}  // namespace MGDF