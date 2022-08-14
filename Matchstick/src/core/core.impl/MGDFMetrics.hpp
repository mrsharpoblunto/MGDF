#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <map>
#include <mutex>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace MGDF {
namespace core {

class MetricBase : public ComBase<IMGDFMetric> {
 public:
  virtual ~MetricBase() {}
  virtual void Dump(std::ostringstream &output) = 0;
};

template <typename T>
class MetricImpl : public MetricBase {
 public:
  struct MetricStorage {
    T Metric;
    UINT64 Timestamp;
  };

  MetricImpl(const char *name, const char *description)
      : _name(name), _description(description), _hasRecorded(false) {}

  virtual ~MetricImpl() {}

  void __stdcall Record(double value, const small **tags,
                        const small **tagValues, const UINT64 tagCount) final {
    std::map<std::string_view, std::string_view> tagMap;
    for (UINT64 i = 0; i < tagCount; ++i) {
      tagMap.emplace(std::make_pair(std::string_view(tags[i]),
                                    std::string_view(tagValues[i])));
    }

    std::ostringstream tagKey;
    tagKey << "{";
    bool first = true;
    for (auto &it : tagMap) {
      if (!first) {
        tagKey << ",";
      } else {
        first = false;
      }
      tagKey << it.first << "=\"" << it.second << "\"";
    }
    tagKey << "}";

    {
      std::lock_guard lock(_mutex);
      auto found = _metric.find(tagKey.str());
      if (found == _metric.end()) {
        found = _metric
                    .emplace(std::make_pair(tagKey.str(),
                                            MetricStorage{.Timestamp = 0}))
                    .first;
      }
      found->second.Timestamp = std::time(0);
      DoRecord(value, found->second);
      _hasRecorded = true;
    }
  }

  void Dump(std::ostringstream &output) final {
    std::lock_guard lock(_mutex);
    if (_hasRecorded) {
      output << "# HELP " << _description << std::endl;
      DoDump(output, _name, _metric);
      output << std::endl;
    }
  }

 protected:
  virtual void DoRecord(double value, MetricStorage &storage) = 0;
  virtual void DoDump(
      std::ostringstream &output, std::string &name,
      std::unordered_map<std::string, MetricStorage> &storage) = 0;

 private:
  bool _hasRecorded;
  std::string _description;
  std::string _name;
  std::unordered_map<std::string, MetricStorage> _metric;
  std::mutex _mutex;
};

class CounterMetric : public MetricImpl<double> {
 public:
  CounterMetric(const char *name, const char *description)
      : MetricImpl<double>(name, description) {}
  virtual ~CounterMetric() {}

 protected:
  void DoRecord(double value, MetricStorage &storage) final;
  void DoDump(std::ostringstream &output, std::string &name,
              std::unordered_map<std::string, MetricStorage> &storage) final;
};

class GaugeMetric : public MetricImpl<double> {
 public:
  GaugeMetric(const char *name, const char *description)
      : MetricImpl<double>(name, description) {}
  virtual ~GaugeMetric() {}

 protected:
  void DoRecord(double value, MetricStorage &storage) final;
  void DoDump(std::ostringstream &output, std::string &name,
              std::unordered_map<std::string, MetricStorage> &storage) final;
};

struct HistogramStorage {
  std::vector<std::pair<double, UINT64>> Buckets;
  double Sum;
  UINT64 Count;
  HistogramStorage() : Sum(0), Count(0) {}
};

class HistogramMetric : public MetricImpl<HistogramStorage> {
 public:
  HistogramMetric(const char *name, const char *description,
                  const double *buckets, UINT64 bucketLength);
  virtual ~HistogramMetric() {}

 protected:
  void DoRecord(double value, MetricStorage &storage) final;
  void DoDump(std::ostringstream &output, std::string &name,
              std::unordered_map<std::string, MetricStorage> &storage) final;

 private:
  std::vector<std::pair<double, UINT64>> _buckets;
};

}  // namespace core
}  // namespace MGDF
