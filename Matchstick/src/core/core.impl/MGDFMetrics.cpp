#include "StdAfx.h"

#include "MGDFMetrics.hpp"

#include <algorithm>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

const char* S_EMPTY_KEY = "{}";

void CounterMetric::DoDump(
    std::ostringstream& output, std::string& name,
    std::unordered_map<std::string, typename MetricImpl<double>::MetricStorage>&
        storage) {
  output << "# TYPE " << name << " counter" << std::endl;
  for (auto& it : storage) {
    const std::string_view key = it.first == "{}" ? "" : it.first;
    output << name << key << " " << it.second.Metric << " "
           << it.second.Timestamp << std::endl;
  }
}

void CounterMetric::DoRecord(
    double value, typename MetricImpl<double>::MetricStorage& storage) {
  storage.Metric += value;
}

void GaugeMetric::DoDump(
    std::ostringstream& output, std::string& name,
    std::unordered_map<std::string, typename MetricImpl<double>::MetricStorage>&
        storage) {
  output << "# TYPE " << name << " gauge" << std::endl;
  for (auto& it : storage) {
    const std::string_view key = it.first == "{}" ? "" : it.first;
    output << name << key << " " << it.second.Metric << " "
           << it.second.Timestamp << std::endl;
  }
}

void GaugeMetric::DoRecord(
    double value, typename MetricImpl<double>::MetricStorage& storage) {
  storage.Metric = value;
}

HistogramMetric::HistogramMetric(const char* name, const char* description,
                                 const double* buckets, UINT64 bucketLength)
    : MetricImpl<HistogramStorage>(name, description) {
  for (UINT64 i = 0; i < bucketLength; ++i) {
    _buckets.emplace_back(std::make_pair(buckets[i], 0U));
  }
  std::sort(_buckets.begin(), _buckets.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });
  _buckets.emplace_back(std::make_pair(DBL_MAX, 0U));
}

void HistogramMetric::DoRecord(
    double value,
    typename MetricImpl<HistogramStorage>::MetricStorage& storage) {
  if (!storage.Metric.Buckets.size()) {
    storage.Metric.Buckets = _buckets;
  }
  for (auto& b : storage.Metric.Buckets) {
    if (value <= b.first) {
      b.second++;
    }
  }
  storage.Metric.Count++;
  storage.Metric.Sum += value;
}

void HistogramMetric::DoDump(
    std::ostringstream& output, std::string& name,
    std::unordered_map<std::string,
                       typename MetricImpl<HistogramStorage>::MetricStorage>&
        storage) {
  output << "# TYPE " << name << " histogram" << std::endl;
  for (auto& it : storage) {
    const std::string_view key = it.first == "{}" ? "" : it.first;
    size_t i = 0;
    for (auto& b : it.second.Metric.Buckets) {
      const auto last = i == it.second.Metric.Buckets.size() - 1;
      output << name << "_bucket{le=\""
             << (last ? "+Inf" : std::to_string(b.first)) << "\""
             << (key.size() > 2 ? "," : "") << key.substr(1) << " " << b.second
             << std::endl;
      ++i;
    }
    output << name << "_sum" << key << " " << it.second.Metric.Sum << std::endl;
    output << name << "_count" << key << " " << it.second.Metric.Count
           << std::endl;
  }
}
}  // namespace core
}  // namespace MGDF
