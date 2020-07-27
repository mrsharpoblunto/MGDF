#include "StdAfx.h"

#include "MGDFStatisticsManagerImpl.hpp"

#include <fstream>
#include <iostream>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

#define SEND_THRESHOLD 25

StatisticsManager::StatisticsManager() : _startTime(time(NULL)) {
  _statisticsFile = Resources::Instance().GameUserStatisticsFile();

  std::ofstream file(_statisticsFile.c_str(),
                     std::ios_base::out | std::ios_base::trunc);
  file.close();
  LOG("StatisticsManager enabled", MGDF_LOG_LOW);
}

StatisticsManager::~StatisticsManager() {
  if (_saveBuffer.size() > 0) {
    LOG("Saving remaining statistics...", MGDF_LOG_LOW);
    SaveAll();
  }
}

HRESULT StatisticsManager::SaveStatistic(const char* name, const char* value) {
  if (!name || strchr(name, ' ') != NULL || strlen(name) > 255)
    return E_INVALIDARG;
  if (!value || strlen(value) > 255) return E_INVALIDARG;

  _saveBuffer.push_back(std::tuple<time_t, std::string, std::string>(
      time(NULL) - _startTime, name, value));

  if (_saveBuffer.size() >= SEND_THRESHOLD) {
    SaveAll();
  }
  return S_OK;
}

void StatisticsManager::SaveAll() {
  std::ofstream file(_statisticsFile.c_str(),
                     std::ios_base::out | std::ios_base::app);
  if (!file.bad() && file.is_open()) {
    for (auto& pair : _saveBuffer) {
      file << std::get<0>(pair) << ":" << std::get<1>(pair) << " "
           << std::get<2>(pair) << "\r\n";
    }
    file.close();

  } else {
    LOG("Error saving statistics to " << Resources::ToString(_statisticsFile),
        MGDF_LOG_ERROR);
  }
  _saveBuffer.clear();
}

}  // namespace core
}  // namespace MGDF