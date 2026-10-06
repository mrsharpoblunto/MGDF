#include "Stdafx.h"

#include "MGDFDebugImpl.hpp"

#include "../common/MGDFVersionInfo.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

Debug::Debug(Timer* timer) : _timer(timer), _metrics(nullptr) {
  _shown.store(false);
}

void Debug::SetMetrics(const HostMetrics* metrics) { _metrics = metrics; }

void Debug::Set(const char* section, const char* key, const char* value) {
  if (!section || !key || !value) {
    return;
  }
  std::lock_guard<std::mutex> lock(_dataMutex);

  auto sectionMap = _data.find(section);
  if (sectionMap == _data.end()) {
    sectionMap = _data
                     .insert(std::make_pair(
                         section, std::map<std::string, std::string>()))
                     .first;
  }
  sectionMap->second[key] = value;
}

void Debug::Clear(const char* section, const char* key) {
  if (!section) {
    return;
  }
  std::lock_guard<std::mutex> lock(_dataMutex);
  if (!key) {
    _data.erase(section);
  } else {
    const auto sectionMap = _data.find(section);
    if (sectionMap == _data.end()) {
      return;
    }
    sectionMap->second.erase(key);
  }
}

BOOL Debug::IsShown() {
  bool exp = true;
  return _shown.compare_exchange_weak(exp, true);
}

void Debug::ToggleShown() {
  // Toggle  stats overlay with alt f12
  bool exp = true;
  // if its true set it to false.
  if (!_shown.compare_exchange_strong(exp, false)) {
    // otherwise it must be false so set it to true
    exp = false;
    _shown.compare_exchange_strong(exp, true);
  }
}

HRESULT Debug::GetOverlaySnapshot(IMGDFDebugOverlaySnapshot** snapshot) {
  if (!snapshot) return E_INVALIDARG;
  DebugSections sections;
  {
    std::lock_guard<std::mutex> lock(_dataMutex);
    sections = _data;
  }
  auto copy = MakeCom<DebugOverlaySnapshot>(_metrics, _timer, sections);
  copy.AddRawRef(snapshot);
  return S_OK;
}

MGDFDebugTiming DebugOverlaySnapshot::View(double average,
                                           const std::vector<double>& samples) {
  return {average, samples.data(), samples.size()};
}

DebugOverlaySnapshot::DebugOverlaySnapshot(const HostMetrics* metrics,
                                           const Timer* timer,
                                           const DebugSections& sections)
    : _sections(sections), _data{} {
  _data.Version = MGDFVersionInfo::MGDF_VERSION();
  _data.InterfaceVersion = MGDFVersionInfo::MGDF_INTERFACE_VERSION;

  if (metrics) {
    Timings timings;
    metrics->GetTimings(timings);
    metrics->GetSamples(_samples);
    _data.HasTimings = TRUE;
    _data.ExpectedSimTime = timings.ExpectedSimTime;
    _data.RenderTime = View(timings.AvgRenderTime, _samples.RenderTime);
    _data.ActiveRenderTime =
        View(timings.AvgActiveRenderTime, _samples.ActiveRenderTime);
    _data.SimTime = View(timings.AvgSimTime, _samples.SimTime);
    _data.ActiveSimTime =
        View(timings.AvgActiveSimTime, _samples.ActiveSimTime);
    _data.SimInputTime = View(timings.AvgSimInputTime, _samples.SimInputTime);
    _data.SimAudioTime = View(timings.AvgSimAudioTime, _samples.SimAudioTime);
  }

  if (timer) timer->GetCounterSnapshots(_counters);
  _counterViews.reserve(_counters.size());
  for (const auto& c : _counters) {
    _counterViews.push_back(
        {c.Name.c_str(), c.GPU ? TRUE : FALSE, View(c.Average, c.Samples)});
  }
  _data.Counters = _counterViews.data();
  _data.CounterCount = _counterViews.size();

  size_t count = 0;
  for (const auto& section : _sections) count += section.second.size();
  _entries.reserve(count);
  for (const auto& section : _sections) {
    for (const auto& kvp : section.second) {
      _entries.push_back(
          {section.first.c_str(), kvp.first.c_str(), kvp.second.c_str()});
    }
  }
  _data.Entries = _entries.data();
  _data.EntryCount = _entries.size();
}

}  // namespace core
}  // namespace MGDF