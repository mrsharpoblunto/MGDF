#include "StdAfx.h"

#include "MGDFHostStats.hpp"

#include "../common/MGDFParameterManager.hpp"
#include "../common/MGDFResources.hpp"
#include "MGDFParameterConstants.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

void HostStatsServer::OnRequest(struct mg_connection* c,
                                struct mg_http_message* m) {
  if (mg_http_match_uri(m, "/metrics")) {
    {
      std::lock_guard lock(_metricsMutex);
      if (_updateResponse) {
        std::ostringstream oss;
        for (auto& it : _metrics) {
          it->DumpPrometheus(oss);
        }
        _rawResponse = oss.str();
        Resources::CompressString(oss.str(), _compressedResponse);
        _updateResponse = false;
      }
    }
    const auto acceptEncodingHeader = mg_http_get_header(m, "Accept-Encoding");
    std::string encoding("identity");
    if (acceptEncodingHeader) {
      const std::string_view acceptEncoding(acceptEncodingHeader->ptr,
                                            acceptEncodingHeader->len);
      if (acceptEncoding.find("gzip") != std::string_view::npos) {
        encoding = "gzip";
      }
    }

    mg_printf(
        c,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; version=0.0.4\r\n"
        "Content-Encoding: %s\r\n"
        "Content-Length: %d\r\n"
        "\r\n",
        encoding.c_str(),
        encoding == "gzip" ? _compressedResponse.size() : _rawResponse.size());

    if (encoding == "gzip") {
      mg_send(c, _compressedResponse.data(), _compressedResponse.size());
    } else {
      mg_send(c, _rawResponse.data(), _rawResponse.size());
    }
  } else {
    mg_http_reply(c, 404, "", "Not Found");
  }
}

void HostStatsServer::UpdateResponse(
    std::unordered_map<std::string, MetricBase*>& metrics) {
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

HostStats::HostStats(UINT32 maxSamples)
    : _avgActiveRenderTime(0),
      _avgRenderTime(0),
      _avgActiveSimTime(0),
      _avgSimTime(0),
      _expectedSimTime(0),
      _avgSimInputTime(0),
      _avgSimAudioTime(0) {
  _maxSamples = maxSamples;
  auto metricsPort = ParameterManager::Instance().GetParameter(
      ParameterConstants::METRICS_PORT);
  if (metricsPort) {
    _server.Listen(metricsPort);
  }
}

void HostStats::UpdateMetrics(
    std::unordered_map<std::string, MetricBase*>& metrics) {
  std::ostringstream oss;
  if (_server.Listening()) {
    _server.UpdateResponse(metrics);
  }
}

void HostStats::GetTimings(Timings& timings) const {
  std::lock_guard<std::mutex> lock(_statsMutex);
  timings.AvgActiveRenderTime = _avgActiveRenderTime / _maxSamples;
  timings.AvgRenderTime = _avgRenderTime / _maxSamples;
  timings.AvgActiveSimTime = _avgActiveSimTime / _maxSamples;
  timings.AvgSimTime = _avgSimTime / _maxSamples;
  timings.AvgSimInputTime = _avgSimInputTime / _maxSamples;
  timings.AvgSimAudioTime = _avgSimAudioTime / _maxSamples;
  timings.ExpectedSimTime = _expectedSimTime;
}

void HostStats::SetExpectedSimTime(double value) { _expectedSimTime = value; }

double HostStats::ExpectedSimTime() const { return _expectedSimTime; }

void HostStats::AppendRenderTimes(double renderValue,
                                  double activeRenderValue) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(renderValue, _avgRenderTime, _renderTime);
  Append(activeRenderValue, _avgActiveRenderTime, _activeRenderTime);
}

void HostStats::AppendActiveSimTime(double value) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(value - *_simInputTime.begin() - *_simAudioTime.begin(),
         _avgActiveSimTime, _activeSimTime);
}

void HostStats::AppendSimTime(double value) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(value, _avgSimTime, _simTime);
}

void HostStats::AppendSimInputAndAudioTimes(double inputValue,
                                            double audioValue) {
  std::lock_guard<std::mutex> lock(_statsMutex);
  Append(inputValue, _avgSimInputTime, _simInputTime);
  Append(audioValue, _avgSimAudioTime, _simAudioTime);
}

void HostStats::Append(double value, double& averageValue,
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