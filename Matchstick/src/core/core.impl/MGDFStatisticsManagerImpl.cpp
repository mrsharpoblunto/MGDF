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
  LOG("StatisticsManager enabled", LOG_LOW);
}

StatisticsManager::~StatisticsManager() {
  if (_saveBuffer.size() > 0) {
    LOG("Saving remaining statistics...", LOG_LOW);
    SaveAll();
  }
}

MGDFError StatisticsManager::SaveStatistic(const char* name,
                                           const char* value) {
  if (!name || strchr(name, ' ') != NULL || strlen(name) > 255)
    return MGDF_ERR_INVALID_STATS_KEY;
  if (!value || strlen(value) > 255) return MGDF_ERR_INVALID_STATS_VALUE;

  _saveBuffer.push_back(std::tuple<time_t, std::string, std::string>(
      time(NULL) - _startTime, name, value));

  if (_saveBuffer.size() >= SEND_THRESHOLD) {
    SaveAll();
  }
  return MGDF_OK;
}

void StatisticsManager::SaveAll() {
  try {
    std::ofstream file(_statisticsFile.c_str(),
                       std::ios_base::out | std::ios_base::app);
    for (auto& pair : _saveBuffer) {
      file << std::get<0>(pair) << ":" << std::get<1>(pair) << " "
           << std::get<2>(pair) << "\r\n";
    }
    file.close();
  } catch (const std::exception& e) {
    LOG("Error saving statistics: " << e.what(), LOG_ERROR);
  }
  _saveBuffer.clear();
}

}  // namespace core
}  // namespace MGDF