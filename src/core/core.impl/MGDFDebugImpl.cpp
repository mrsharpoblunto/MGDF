#include "Stdafx.h"

#include "MGDFDebugImpl.hpp"

#include <cmath>
#include <cstdio>
#include <iomanip>

#include "../common/MGDFResources.hpp"
#include "../common/MGDFStringImpl.hpp"
#include "../common/MGDFVersionInfo.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

namespace {

void WriteJsonString(std::ostringstream& out, const std::string& value) {
  out << '"';
  for (const unsigned char c : value) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
              << static_cast<int>(c) << std::dec;
        } else {
          out << c;
        }
        break;
    }
  }
  out << '"';
}

void WriteJsonNumber(std::ostringstream& out, double value) {
  if (!std::isfinite(value)) {
    out << "null";
    return;
  }
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "%.9g", value);
  out << buffer;
}

void WriteJsonNumbers(std::ostringstream& out, const std::vector<double>& v) {
  out << '[';
  for (size_t i = 0; i < v.size(); ++i) {
    if (i) out << ',';
    WriteJsonNumber(out, v[i]);
  }
  out << ']';
}

}  // namespace

Debug::Debug(Timer* timer) : _timer(timer), _metrics(nullptr) {
  _shown.store(false);
  _hostRendering.store(true);
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

void Debug::SetHostRenderingEnabled(BOOL enabled) {
  _hostRendering.store(enabled != FALSE);
}

BOOL Debug::IsHostRenderingEnabled() { return _hostRendering.load(); }

HRESULT Debug::GetOverlayData(char* buffer, UINT64* length) {
  if (!length) return E_INVALIDARG;
  const std::string json = BuildOverlayData();
  if (buffer && *length < json.size()) {
    *length = json.size();
    return E_NOT_SUFFICIENT_BUFFER;
  }
  size_t size = static_cast<size_t>(*length);
  const HRESULT result = StringWriter::Write(json, buffer, &size);
  *length = size;
  return result;
}

std::string Debug::BuildOverlayData() const {
  std::ostringstream out;
  out << "{\"version\":";
  WriteJsonString(out, MGDFVersionInfo::MGDF_VERSION());
  out << ",\"interfaceVersion\":" << MGDFVersionInfo::MGDF_INTERFACE_VERSION;

  if (_metrics) {
    Timings timings;
    _metrics->GetTimings(timings);
    TimingSamples samples;
    _metrics->GetSamples(samples);
    out << ",\"timings\":{\"expectedSimTime\":";
    WriteJsonNumber(out, timings.ExpectedSimTime);
    out << ",\"render\":{\"avg\":";
    WriteJsonNumber(out, timings.AvgRenderTime);
    out << ",\"activeAvg\":";
    WriteJsonNumber(out, timings.AvgActiveRenderTime);
    out << ",\"samples\":";
    WriteJsonNumbers(out, samples.RenderTime);
    out << ",\"activeSamples\":";
    WriteJsonNumbers(out, samples.ActiveRenderTime);
    out << "},\"sim\":{\"avg\":";
    WriteJsonNumber(out, timings.AvgSimTime);
    out << ",\"activeAvg\":";
    WriteJsonNumber(out, timings.AvgActiveSimTime);
    out << ",\"inputAvg\":";
    WriteJsonNumber(out, timings.AvgSimInputTime);
    out << ",\"audioAvg\":";
    WriteJsonNumber(out, timings.AvgSimAudioTime);
    out << ",\"samples\":";
    WriteJsonNumbers(out, samples.SimTime);
    out << ",\"activeSamples\":";
    WriteJsonNumbers(out, samples.ActiveSimTime);
    out << ",\"inputSamples\":";
    WriteJsonNumbers(out, samples.SimInputTime);
    out << ",\"audioSamples\":";
    WriteJsonNumbers(out, samples.SimAudioTime);
    out << "}}";
  }

  out << ",\"counters\":[";
  if (_timer) {
    std::vector<CounterSnapshot> counters;
    _timer->GetCounterSnapshots(counters);
    for (size_t i = 0; i < counters.size(); ++i) {
      if (i) out << ',';
      out << "{\"name\":";
      WriteJsonString(out, counters[i].Name);
      out << ",\"gpu\":" << (counters[i].GPU ? "true" : "false")
          << ",\"average\":";
      WriteJsonNumber(out, counters[i].Average);
      out << ",\"samples\":";
      WriteJsonNumbers(out, counters[i].Samples);
      out << '}';
    }
  }
  out << "],\"sections\":{";
  {
    std::lock_guard<std::mutex> lock(_dataMutex);
    bool firstSection = true;
    for (const auto& section : _data) {
      if (!firstSection) out << ',';
      firstSection = false;
      WriteJsonString(out, section.first);
      out << ":{";
      bool firstKey = true;
      for (const auto& kvp : section.second) {
        if (!firstKey) out << ',';
        firstKey = false;
        WriteJsonString(out, kvp.first);
        out << ':';
        WriteJsonString(out, kvp.second);
      }
      out << '}';
    }
  }
  out << "}}";
  return out.str();
}

void Debug::DumpInfo(const HostMetrics& stats, TextStream& ss) const {
  std::wstring mgdfVersion(
      Resources::ToWString(MGDFVersionInfo::MGDF_VERSION()));

  Timings timings;
  stats.GetTimings(timings);

  ss.SetF(std::ios::fixed);

  ss << TextStyle::Weight(DWRITE_FONT_WEIGHT_BOLD)
     << "MGDF Version: " << TextStyle::Pop() << mgdfVersion
     << TextStyle::Weight(DWRITE_FONT_WEIGHT_BOLD)
     << "\r\nMGDF Interface version:" << TextStyle::Pop()
     << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\n";

  ss << TextStyle::Weight(DWRITE_FONT_WEIGHT_BOLD) << "\r\nRender Thread\r\n"
     << TextStyle::Pop();
  ss.Precision(0);
  ss << " FPS : ";
  if (timings.AvgRenderTime == 0)
    ss << "N/A\r\n";
  else
    ss << 1 / timings.AvgRenderTime << "\r\n";

  ss.Precision(1);
  ss << " Render CPU : " << timings.AvgActiveRenderTime * 1000 << "\r\n";
  ss << " Idle CPU : "
     << (timings.AvgRenderTime - timings.AvgActiveRenderTime) * 1000 << "\r\n";

  ss << TextStyle::Weight(DWRITE_FONT_WEIGHT_BOLD) << "\r\nSim Thread\r\n"
     << TextStyle::Pop();
  ss.Precision(0);
  ss << " Expected FPS : ";
  if (timings.ExpectedSimTime == 0)
    ss << "N/A\r\n";
  else
    ss << 1 / timings.ExpectedSimTime << "\r\n";

  ss << " Actual FPS : ";
  if (timings.AvgSimTime == 0)
    ss << "N/A";
  else
    ss << 1 / timings.AvgSimTime;

  ss.Precision(1);
  std::set<std::pair<std::string, double>> simTimings;
  simTimings.insert(std::make_pair("Input CPU", timings.AvgSimInputTime));
  simTimings.insert(std::make_pair("Audio CPU", timings.AvgSimAudioTime));
  simTimings.insert(std::make_pair("Other CPU", timings.AvgActiveSimTime));
  simTimings.insert(std::make_pair(
      "Idle CPU", (timings.AvgSimTime - timings.AvgActiveSimTime -
                   timings.AvgSimInputTime - timings.AvgSimAudioTime)));

  ss.Precision(2);
  KeyValueHeatMap<std::pair<std::string, double>, double>(
      simTimings,
      [](const auto& in, auto& out) {
        out.first = in.first;
        out.second = in.second * 1000;
      },
      ss);

  std::lock_guard<std::mutex> lock(_dataMutex);
  for (auto section = _data.cbegin(); section != _data.cend(); ++section) {
    ss << "\r\n\r\n"
       << TextStyle::Weight(DWRITE_FONT_WEIGHT_BOLD)
       << Resources::ToWString(section->first) << TextStyle::Pop();

    for (auto kvp = section->second.cbegin(); kvp != section->second.cend();
         ++kvp) {
      ss << "\r\n " << Resources::ToWString(kvp->first) << " : "
         << Resources::ToWString(kvp->second);
    }
  }
}

}  // namespace core
}  // namespace MGDF