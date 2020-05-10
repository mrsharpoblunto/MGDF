#pragma once

#include <time.h>

#include <MGDF/MGDFStatisticsManager.hpp>
#include <vector>

namespace MGDF {
namespace core {

class StatisticsManager : public IStatisticsManager {
 public:
  StatisticsManager();
  virtual ~StatisticsManager();
  MGDFError SaveStatistic(const char* name, const char* value) override final;

 private:
  std::vector<std::tuple<time_t, std::string, std::string> > _saveBuffer;
  std::wstring _statisticsFile;
  void SaveAll();
  time_t _startTime;
};

}  // namespace core
}  // namespace MGDF