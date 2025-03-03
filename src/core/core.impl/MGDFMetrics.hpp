#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <chrono>
#include <map>
#include <mutex>
#include <sstream>
#include <string_view>
#include <unordered_map>

namespace MGDF {
namespace core {

struct PushStatistic {
  size_t Timestamp = 0U;
  std::string Name;
  std::string Value;
  std::map<std::string, std::string> Tags;
};

class MetricBase : public ComBase<IMGDFMetric> {
 public:
  virtual ~MetricBase() {}
  virtual void DumpPrometheus(std::ostringstream &output) const = 0;
  virtual void DumpPush(PushStatistic &stat) const = 0;
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

  void __stdcall Record(double value, const MGDFTags *tags) final {
    std::ostringstream tagKey;

    if (tags) {
      std::map<std::string_view, std::string_view> tagMap;
      for (UINT64 i = 0; i < tags->Count; ++i) {
        tagMap.emplace(std::make_pair(std::string_view(tags->Names[i]),
                                      std::string_view(tags->Values[i])));
      }

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
    } else {
      tagKey << "{}";
    }

    {
      std::lock_guard lock(_mutex);
      auto found = _metric.find(tagKey.str());
      if (found == _metric.end()) {
        found = _metric
                    .emplace(std::make_pair(tagKey.str(),
                                            MetricStorage{.Timestamp = 0}))
                    .first;
      }
      found->second.Timestamp =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch())
              .count();
      DoRecord(value, found->second);
      _hasRecorded = true;
    }
  }

  void DumpPrometheus(std::ostringstream &output) const final {
    std::lock_guard lock(_mutex);
    if (_hasRecorded) {
      output << "# HELP " << _description << "\n";
      DoDumpPrometheus(output, _name, _metric);
      output << "\n";
    }
  }

  void DumpPush(PushStatistic &stat) const final {
    stat.Name = _name;
    std::ostringstream output;

    for (const auto &it : _metric) {
      // parse out the tag keys for this metric
      // into different tag sets
      const std::string_view tags(it.first);
      size_t index = 1;
      while (tags[index] != '}') {
        const size_t nameStart = index;
        while (tags[index++] != '=') {
        }
        const size_t nameEnd = index - 1;
        ++index;
        const size_t valueStart = index;
        while (tags[index++] != '\"') {
        }
        const size_t valueEnd = index - 1;

        stat.Tags.insert(
            std::make_pair(tags.substr(nameStart, nameEnd - nameStart),
                           tags.substr(valueStart, valueEnd - valueStart)));

        if (tags[index] == ',') {
          ++index;
        }
      }

      DoDumpPushValue(output, it.second);
    }
    stat.Value = output.str();
  }

 protected:
  virtual void DoRecord(double value, MetricStorage &storage) = 0;
  virtual void DoDumpPrometheus(
      std::ostringstream &output, const std::string &name,
      const std::unordered_map<std::string, MetricStorage> &storage) const = 0;
  virtual void DoDumpPushValue(std::ostringstream &output,
                               const MetricStorage &storage) const = 0;

 private:
  bool _hasRecorded;
  std::string _description;
  std::string _name;
  std::unordered_map<std::string, MetricStorage> _metric;
  mutable std::mutex _mutex;
};

class CounterMetric : public MetricImpl<double> {
 public:
  CounterMetric(const char *name, const char *description)
      : MetricImpl<double>(name, description) {}
  virtual ~CounterMetric() {}

 protected:
  void DoRecord(double value, MetricStorage &storage) final;
  void DoDumpPrometheus(std::ostringstream &output, const std::string &name,
                        const std::unordered_map<std::string, MetricStorage>
                            &storage) const final;
  void DoDumpPushValue(std::ostringstream &output,
                       const MetricStorage &storage) const final;
};

class GaugeMetric : public MetricImpl<double> {
 public:
  GaugeMetric(const char *name, const char *description)
      : MetricImpl<double>(name, description) {}
  virtual ~GaugeMetric() {}

 protected:
  void DoRecord(double value, MetricStorage &storage) final;
  void DoDumpPrometheus(std::ostringstream &output, const std::string &name,
                        const std::unordered_map<std::string, MetricStorage>
                            &storage) const final;
  void DoDumpPushValue(std::ostringstream &output,
                       const MetricStorage &storage) const final;
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
  void DoDumpPrometheus(std::ostringstream &output, const std::string &name,
                        const std::unordered_map<std::string, MetricStorage>
                            &storage) const final;
  void DoDumpPushValue(std::ostringstream &output,
                       const MetricStorage &storage) const final;

 private:
  std::vector<std::pair<double, UINT64>> _buckets;
};

}  // namespace core
}  // namespace MGDF
