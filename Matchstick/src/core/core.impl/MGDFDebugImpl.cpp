#include "Stdafx.h"

#include "MGDFDebugImpl.hpp"

#include "../common/MGDFResources.hpp"
#include "../common/MGDFVersionInfo.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

Debug::Debug(Timer* timer) : _timer(timer) { _shown.store(false); }

void Debug::Set(const char* section, const char* key, const char* value) {
  if (!section || !key || !value) {
    return;
  }

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
  if (!key) {
    _data.erase(section);
  } else {
    auto sectionMap = _data.find(section);
    if (sectionMap == _data.end()) {
      return;
    }
    sectionMap->second.erase(key);
  }
}

bool Debug::IsShown() const {
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

void Debug::DumpInfo(const HostStats& stats, TextStream& ss) const {
  std::wstring mgdfVersion(MGDFVersionInfo::MGDF_VERSION().begin(),
                           MGDFVersionInfo::MGDF_VERSION().end());

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

  ss.Precision(4);
  ss << " Render CPU : " << timings.AvgActiveRenderTime << "\r\n";
  ss << " Idle CPU : " << timings.AvgRenderTime - timings.AvgActiveRenderTime
     << "\r\n";

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

  ss.Precision(4);
  std::set<std::pair<std::string, double>> simTimings;
  simTimings.insert(std::make_pair("Input CPU", timings.AvgSimInputTime));
  simTimings.insert(std::make_pair("Audio CPU", timings.AvgSimAudioTime));
  simTimings.insert(std::make_pair("Other CPU", timings.AvgActiveSimTime));
  simTimings.insert(std::make_pair(
      "Idle CPU", (timings.AvgSimTime - timings.AvgActiveSimTime -
                   timings.AvgSimInputTime - timings.AvgSimAudioTime)));

  KeyValueHeatMap<std::pair<std::string, double>, double>(
      simTimings,
      [](const auto& in, auto& out) {
        out.first = in.first;
        out.second = in.second;
      },
      ss);

  _timer->GetCounterInformation(ss);

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