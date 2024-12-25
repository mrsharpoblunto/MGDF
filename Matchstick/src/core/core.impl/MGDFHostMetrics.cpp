#include "StdAfx.h"

#include "MGDFHostMetrics.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

void HostMetricsServer::OnRequest(std::shared_ptr<HttpServerRequest>& request) {
  if (request->GetRequestUrl() == "/metrics") {
    {
      std::lock_guard lock(_metricsMutex);
      if (_updateResponse) {
        std::ostringstream oss;
        for (auto& it : _metrics) {
          it->DumpPrometheus(oss);
        }
        _rawResponse = oss.str();
        _updateResponse = false;
      }
    }
    request->SetResponseCode(200)
        ->SetResponseHeader("Content-Type", "text/plain; version=0.0.4")
        ->SetResponseBody(_rawResponse.data(), _rawResponse.size())
        ->SendResponse();
  } else {
    request->SetResponseCode(404)->SendResponse();
  }
}

void HostMetricsServer::UpdateResponse(
    std::unordered_map<std::string, MetricBase*>& metrics) {
  if (!Listening()) {
    return;
  }
  std::lock_guard lock(_metricsMutex);
  _updateResponse = true;
  // metrics only ever get added, not removed
  // so a size comparison is enough to know if there have
  // been any changes
  if (_metrics.size() != metrics.size()) {
    _metrics.clear();
    for (auto& it : metrics) {
      _metrics.push_back(MakeComFromPtr<MetricBase>(it.second));
    }
  }
}

HostMetrics::HostMetrics(UINT32 maxSamples)
    : _avgActiveRenderTime(0),
      _avgRenderTime(0),
      _avgActiveSimTime(0),
      _avgSimTime(0),
      _expectedSimTime(0),
      _avgSimInputTime(0),
      _avgSimAudioTime(0) {
  _maxSamples = maxSamples;
}

void HostMetrics::GetTimings(Timings& timings) const {
  std::lock_guard<std::mutex> lock(_statsMutex);
  timings.AvgActiveRenderTime = _avgActiveRenderTime / _maxSamples;
  timings.AvgRenderTime = _avgRenderTime / _maxSamples;
  timings.AvgActiveSimTime = _avgActiveSimTime / _maxSamples;
  timings.AvgSimTime = _avgSimTime / _maxSamples;
  timings.AvgSimInputTime = _avgSimInputTime / _maxSamples;
  timings.AvgSimAudioTime = _avgSimAudioTime / _maxSamples;
  timings.ExpectedSimTime = _expectedSimTime;
}

void HostMetrics::SetExpectedSimTime(double value) { _expectedSimTime = value; }

double HostMetrics::ExpectedSimTime() const { return _expectedSimTime; }

void HostMetrics::AppendRenderTimes(double renderValue,
                                    double activeRenderValue) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(renderValue, _avgRenderTime, _renderTime);
  Append(activeRenderValue, _avgActiveRenderTime, _activeRenderTime);
}

void HostMetrics::AppendActiveSimTime(double value) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(value - *_simInputTime.begin() - *_simAudioTime.begin(),
         _avgActiveSimTime, _activeSimTime);
}

void HostMetrics::AppendSimTime(double value) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(value, _avgSimTime, _simTime);
}

void HostMetrics::AppendSimInputAndAudioTimes(double inputValue,
                                              double audioValue) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(inputValue, _avgSimInputTime, _simInputTime);
  Append(audioValue, _avgSimAudioTime, _simAudioTime);
}

void HostMetrics::Append(double value, double& averageValue,
                         std::list<double>& list) {
  list.push_front(value);
  averageValue += value;
  if (list.size() > _maxSamples) {
    averageValue -= list.back();
    list.pop_back();
  }
}

}  // namespace core
}  // namespace MGDF