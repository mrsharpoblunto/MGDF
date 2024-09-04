#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <vector>

#include "../common/MGDFHttpClient.hpp"
#include "MGDFMetrics.hpp"

namespace MGDF {
namespace core {

class StatisticsManager : public ComBase<IMGDFStatisticsManager> {
 public:
  StatisticsManager(const std::string &gameUid,
                    const std::shared_ptr<HttpClient> &client);
  virtual ~StatisticsManager();
  void __stdcall PushString(const char *name, const char *value,
                            const MGDFTags *tags) final;
  void __stdcall PushMetric(IMGDFMetric *metric) final;

  void SetRemoteEndpoint(const std::string &endpoint);

 private:
  PushStatistic &PushCommon(PushStatistic &stat, const MGDFTags *tags);

  std::shared_ptr<HttpClient> _client;
  std::mutex _mutex;
  std::condition_variable _cv;
  std::vector<PushStatistic> _events;
  std::thread _flushThread;
  bool _run;
  bool _enabled;
  std::string _gameUid;
  std::string _sessionId;
  std::string _remoteEndpoint;
  uint64_t _sessionStart;
};

}  // namespace core
}  // namespace MGDF