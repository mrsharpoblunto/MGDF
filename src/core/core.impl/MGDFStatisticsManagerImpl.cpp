#include "StdAfx.h"

#include "MGDFStatisticsManagerImpl.hpp"

#include <chrono>
#include <nlohmann/json.hpp>
#include <sstream>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"
#include "../network/MGDFNetworkManagerComponent.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

using namespace std::chrono_literals;

namespace MGDF {
namespace core {

#define SEND_THRESHOLD 25
#define STAT_FLUSH_TIMEOUT 5s

StatisticsManager::StatisticsManager(
    const std::shared_ptr<network::INetworkManagerComponent>& network,
    const std::string& gameUid)
    : _gameUid(gameUid),
      _network(network),
      _sessionStart(std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count()),
      _enabled(false),
      _run(true) {}

void StatisticsManager::SetRemoteEndpoint(const std::string& endpoint) {
  _remoteEndpoint = endpoint;
  if (_enabled) return;

  GUID guid;
  if (CoCreateGuid(&guid) == S_OK) {
    wchar_t buffer[40] = {0};
    const auto length = StringFromGUID2(guid, buffer, 40);
    if (length != 0) {
      LOG("StatisticsManager enabled", MGDF_LOG_LOW);
      _sessionId = Resources::ToString(buffer).substr(
          1, static_cast<size_t>(length) - 3);
      _enabled = true;
    }
  }

  if (!_enabled) {
    LOG("StatisticsManager could create session ID, will be disabled",
        MGDF_LOG_ERROR);
  } else {
    _run = true;
    _flushThread = std::thread([this]() {
      std::unique_lock<std::mutex> lock(_mutex);
      while (_run) {
        _cv.wait_for(lock, STAT_FLUSH_TIMEOUT,
                     [this] { return !_run || _events.size() > 0; });
        std::vector<PushStatistic> tmp(std::move(_events));
        _events.clear();
        lock.unlock();

        if (tmp.size()) {
          nlohmann::json root;
          root["gameUid"] = _gameUid;
          root["sessionId"] = _sessionId;
          auto& streams = root["statistics"] = nlohmann::json::array();
          for (const auto& s : tmp) {
            auto& statistic = streams.emplace_back(nlohmann::json::object());
            statistic["name"] = s.Name;
            statistic["value"] = s.Value;
            statistic["timestamp"] = static_cast<double>(s.Timestamp);
            auto& tags = statistic["tags"] = nlohmann::json::object();
            for (const auto& t : s.Tags) {
              tags[t.first] = t.second;
            }
          }

          std::ostringstream requestBody;
          requestBody << root;

          const auto endpoint(_remoteEndpoint);
          _network->CreateHttpRequest(_remoteEndpoint)
              ->SetRequestMethod("POST")
              ->SetRequestHeader("Content-Type", "application/json")
              ->SetRequestBody(requestBody.str().c_str(),
                               requestBody.str().size(), true)
              ->SendRequest([endpoint](auto response) {
                if (response->Code != 200) {
                  LOG("Unable to send statistic to remote endpoint "
                          << endpoint << ". status=" << response->Code
                          << ", error=" << response->Error,
                      MGDF_LOG_ERROR);
                }
              });
        }

        lock.lock();
      }
    });
  }
}

StatisticsManager::~StatisticsManager() {
  // force whatever is in the events buffer to be flushed & stop
  // the flush thread running after its finished this last flush
  _run = false;
  if (_flushThread.joinable()) {
    _cv.notify_one();
    _flushThread.join();
  }
}

void StatisticsManager::PushString(const char* name, const char* value,
                                   const MGDFTags* tags) {
  if (!_enabled) return;
  std::unique_lock lock(_mutex);
  PushCommon(_events.emplace_back(PushStatistic{.Name = name, .Value = value}),
             tags);
  if (_events.size() >= SEND_THRESHOLD) {
    lock.unlock();
    _cv.notify_one();
  }
}

void StatisticsManager::PushMetric(IMGDFMetric* metric) {
  if (!_enabled) return;
  std::unique_lock lock(_mutex);
  auto& stat = PushCommon(_events.emplace_back(), nullptr);
  const auto base = dynamic_cast<MetricBase*>(metric);
  if (base) {
    base->DumpPush(stat);
  }
  if (_events.size() >= SEND_THRESHOLD) {
    lock.unlock();
    _cv.notify_one();
  }
}

PushStatistic& StatisticsManager::PushCommon(PushStatistic& stat,
                                             const MGDFTags* tags) {
  stat.Timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now().time_since_epoch())
                       .count() -
                   _sessionStart;
  if (tags) {
    for (size_t i = 0; i < tags->Count; ++i) {
      stat.Tags.insert(std::make_pair(tags->Names[i], tags->Values[i]));
    }
  }
  return stat;
}

}  // namespace core
}  // namespace MGDF