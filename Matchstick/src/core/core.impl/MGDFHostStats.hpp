#pragma once

#include <MGDF/ComObject.hpp>
#include <list>
#include <mutex>
#include <unordered_map>

#include "../common/MGDFHttpServer.hpp"
#include "MGDFMetrics.hpp"

namespace MGDF {
namespace core {

class HostStatsServer : public HttpServer {
 public:
  virtual ~HostStatsServer() {}
  HostStatsServer() : _updateResponse(false) {}
  void OnRequest(struct mg_connection *c, struct mg_http_message *m) final;
  void UpdateResponse(std::unordered_map<std::string, MetricBase *> &metrics);

 private:
  std::string _response;
  bool _updateResponse;
  std::vector<ComObject<MetricBase>> _metrics;
  std::mutex _metricsMutex;
};

struct Timings {
  double AvgActiveRenderTime;
  double AvgRenderTime;
  double AvgActiveSimTime;
  double AvgSimTime;
  double AvgSimInputTime;
  double AvgSimAudioTime;
  double ExpectedSimTime;
};

class HostStats {
 public:
  HostStats(UINT32 maxSamples);
  virtual ~HostStats(){};

  void GetTimings(Timings &timings) const;
  double ExpectedSimTime() const;

  void UpdateMetrics(std::unordered_map<std::string, MetricBase *> &metrics);

  void AppendRenderTimes(double renderValue, double activeRenderValue);
  void SetExpectedSimTime(double value);
  void AppendActiveSimTime(double value);
  void AppendSimTime(double value);
  void AppendSimInputAndAudioTimes(double inputValue, double audioValue);

 private:
  HostStatsServer _server;
  mutable std::mutex _statsMutex;
  UINT32 _maxSamples;
  double _expectedSimTime;

  std::list<double> _activeRenderTime;
  std::list<double> _renderTime;
  std::list<double> _activeSimTime;
  std::list<double> _simTime;
  std::list<double> _simInputTime;
  std::list<double> _simAudioTime;

  double _avgActiveRenderTime;
  double _avgRenderTime;
  double _avgActiveSimTime;
  double _avgSimTime;
  double _avgSimInputTime;
  double _avgSimAudioTime;

  void Append(double value, double &average, std::list<double> &list);
};

}  // namespace core
}  // namespace MGDF