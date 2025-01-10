#pragma once

#include <MGDF/ComObject.hpp>
#include <list>
#include <mutex>
#include <unordered_map>

#include "../network/MGDFNetworkManagerComponent.hpp"
#include "MGDFMetrics.hpp"

namespace MGDF {
namespace core {

class HostMetricsServer {
 public:
  virtual ~HostMetricsServer() {}
  HostMetricsServer(
      std::shared_ptr<network::INetworkManagerComponent> &network);

  void UpdateResponse(std::unordered_map<std::string, MetricBase *> &metrics);
  void Listen(UINT32 port);

 private:
  void OnRequest(std::shared_ptr<network::IHttpServerRequest> request);

  std::string _rawResponse;
  bool _updateResponse;
  std::shared_ptr<network::INetworkManagerComponent> _network;
  std::shared_ptr<network::IHttpServer> _server;
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

class HostMetrics {
 public:
  HostMetrics(UINT32 maxSamples);
  virtual ~HostMetrics() {};

  void GetTimings(Timings &timings) const;
  double ExpectedSimTime() const;

  void AppendRenderTimes(double renderValue, double activeRenderValue);
  void SetExpectedSimTime(double value);
  void AppendActiveSimTime(double value);
  void AppendSimTime(double value);
  void AppendSimInputAndAudioTimes(double inputValue, double audioValue);

 private:
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