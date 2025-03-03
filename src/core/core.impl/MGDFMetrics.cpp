#include "StdAfx.h"

#include "MGDFMetrics.hpp"

#include <algorithm>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

namespace MGDF {
namespace core {

const char* S_EMPTY_KEY = "{}";

void CounterMetric::DoDumpPrometheus(
    std::ostringstream& output, const std::string& name,
    const std::unordered_map<std::string, MetricStorage>& storage) const {
  output << "# TYPE " << name << " counter"
         << "\n";
  for (const auto& it : storage) {
    const char* key = it.first == "{}" ? "" : it.first.c_str();
    output << name << key << " " << it.second.Metric << " "
           << it.second.Timestamp << "\n";
  }
}

void CounterMetric::DoDumpPushValue(std::ostringstream& output,
                                    const MetricStorage& storage) const {
  output << storage.Metric;
}

void CounterMetric::DoRecord(double value, MetricStorage& storage) {
  storage.Metric += value;
}

void GaugeMetric::DoDumpPrometheus(
    std::ostringstream& output, const std::string& name,
    const std::unordered_map<std::string, MetricStorage>& storage) const {
  output << "# TYPE " << name << " gauge"
         << "\n";
  for (const auto& it : storage) {
    const char* key = it.first == "{}" ? "" : it.first.c_str();
    output << name << key << " " << it.second.Metric << " "
           << it.second.Timestamp << "\n";
  }
}

void GaugeMetric::DoDumpPushValue(std::ostringstream& output,
                                  const MetricStorage& storage) const {
  output << storage.Metric;
}

void GaugeMetric::DoRecord(double value, MetricStorage& storage) {
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

void HistogramMetric::DoRecord(double value, MetricStorage& storage) {
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

void HistogramMetric::DoDumpPushValue(std::ostringstream& output,
                                      const MetricStorage& storage) const {
  output << (storage.Metric.Sum / storage.Metric.Count);
}

void HistogramMetric::DoDumpPrometheus(
    std::ostringstream& output, const std::string& name,
    const std::unordered_map<std::string, MetricStorage>& storage) const {
  output << "# TYPE " << name << " histogram"
         << "\n";
  for (const auto& it : storage) {
    const bool emptyKey = it.first == "{}";
    size_t i = 0;
    for (auto& b : it.second.Metric.Buckets) {
      const auto last = i == it.second.Metric.Buckets.size() - 1;
      output << name << "_bucket{le=\""
             << (last ? "+Inf" : std::to_string(b.first)) << "\""
             << (!emptyKey ? ", " : "}")
             << (!emptyKey ? (it.first.c_str() + 1) : "") << " " << b.second
             << "\n";
      ++i;
    }
    const char* key = emptyKey ? "" : it.first.c_str();
    output << name << "_sum" << key << " " << it.second.Metric.Sum << "\n";
    output << name << "_count" << key << " " << it.second.Metric.Count << "\n";
  }
}
}  // namespace core
}  // namespace MGDF
