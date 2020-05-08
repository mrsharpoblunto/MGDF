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

void Debug::DumpInfo(const HostStats& stats, std::wstringstream& ss) const {
  std::wstring mgdfVersion(MGDFVersionInfo::MGDF_VERSION().begin(),
                           MGDFVersionInfo::MGDF_VERSION().end());

  Timings timings;
  stats.GetTimings(timings);

  ss.setf(std::ios::fixed);
  ss.precision(4);

  ss << "MGDF Version: " << mgdfVersion << "\r\nMGDF Interface version: "
     << MGDFVersionInfo::MGDF_INTERFACE_VERSION << "\r\n";
  ss << "\r\nPerformance Statistics:\r\n";

  ss << "Render Thread\r\n";
  ss << " FPS : ";
  if (timings.AvgRenderTime == 0)
    ss << "N/A\r\n";
  else
    ss << 1 / timings.AvgRenderTime << "\r\n";
  ss << " Render CPU : " << timings.AvgActiveRenderTime << "\r\n";
  ss << " Idle CPU : " << timings.AvgRenderTime - timings.AvgActiveRenderTime
     << "\r\n";

  ss << "\r\nSim Thread\r\n";
  ss << " Expected FPS : ";
  if (timings.ExpectedSimTime == 0)
    ss << "N/A\r\n";
  else
    ss << 1 / timings.ExpectedSimTime << "\r\n";

  ss << " Actual FPS : ";
  if (timings.AvgSimTime == 0)
    ss << "N/A\r\n";
  else
    ss << 1 / timings.AvgSimTime << "\r\n";

  ss << " Input CPU : " << timings.AvgSimInputTime << "\r\n";
  ss << " Audio CPU : " << timings.AvgSimAudioTime << "\r\n";
  ss << " Other CPU : " << timings.AvgActiveSimTime << "\r\n";
  ss << " Idle CPU : "
     << (timings.AvgSimTime - timings.AvgActiveSimTime -
         timings.AvgSimInputTime - timings.AvgSimAudioTime)
     << "\r\n";

  _timer->GetCounterInformation(ss);

  for (auto section = _data.cbegin(); section != _data.cend(); ++section) {
    ss << "\r\n" << Resources::ToWString(section->first) << "\r\n";

    for (auto kvp = section->second.cbegin(); kvp != section->second.cend();
         ++kvp) {
      ss << " " << Resources::ToWString(kvp->first) << ": "
         << Resources::ToWString(kvp->second) << "\r\n";
    }
  }
}

}  // namespace core
}  // namespace MGDF