#pragma once

#include <time.h>

#include <MGDF/ComObject.hpp>
#include <MGDF/MGDF.h>
#include <vector>

namespace MGDF {
namespace core {

class StatisticsManager : public ComBase<IMGDFStatisticsManager> {
 public:
  StatisticsManager();
  virtual ~StatisticsManager();
  HRESULT __stdcall SaveStatistic(const char* name, const char* value) final;

 private:
  std::vector<std::tuple<time_t, std::string, std::string> > _saveBuffer;
  std::wstring _statisticsFile;
  void SaveAll();
  time_t _startTime;
};

}  // namespace core
}  // namespace MGDF