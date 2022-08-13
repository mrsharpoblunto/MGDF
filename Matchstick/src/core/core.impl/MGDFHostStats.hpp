#pragma once

#include <list>
#include <mutex>

#include "../common/MGDFHttp.hpp"

namespace MGDF {
namespace core {

class HostStatsServer : public common::HttpServer {
 public:
  virtual ~HostStatsServer() {}
  void OnRequest(struct mg_connection *c, int ev, void *ev_data) final;
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