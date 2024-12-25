#include "StdAfx.h"

#include "MGDFStatisticsManagerImpl.hpp"

#include <json/json.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <set>

#include "../common/MGDFHttpClient.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFResources.hpp"

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
    const std::shared_ptr<NetworkEventLoop>& eventLoop,
    const std::string& gameUid)
    : _gameUid(gameUid),
      _client(std::make_shared<HttpClient>(eventLoop)),
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
      auto pendingRequests = std::make_shared<HttpClientRequestGroup>();
      std::unique_lock<std::mutex> lock(_mutex);
      while (_run) {
        _cv.wait_for(lock, STAT_FLUSH_TIMEOUT, [this, &pendingRequests] {
          return !_run || _events.size() > 0 || pendingRequests->Size() > 0;
        });
        std::vector<PushStatistic> tmp(std::move(_events));
        _events.clear();
        lock.unlock();

        std::shared_ptr<HttpClientResponse> response;
        while (pendingRequests->GetResponse(response)) {
          if (response->Code != 200) {
            LOG("Unable to send statistic to remote endpoint "
                    << _remoteEndpoint << ". status=" << response->Code
                    << ", error=" << response->Error,
                MGDF_LOG_ERROR);
          }
        }

        if (tmp.size()) {
          Json::Value root;
          root["gameUid"] = _gameUid;
          root["sessionId"] = _sessionId;
          Json::Value& streams = root["statistics"] =
              Json::Value(Json::arrayValue);
          for (const auto& s : tmp) {
            Json::Value& statistic =
                streams.append(Json::Value(Json::objectValue));
            statistic["name"] = s.Name;
            statistic["value"] = s.Value;
            statistic["timestamp"] = static_cast<double>(s.Timestamp);
            Json::Value& tags = statistic["tags"] =
                Json::Value(Json::objectValue);
            for (const auto& t : s.Tags) {
              tags[t.first] = t.second;
            }
          }

          auto request = std::make_shared<HttpClientRequest>(_remoteEndpoint);
          std::ostringstream requestBody;
          requestBody << root;
          request->SetMethod("POST")
              ->SetHeader("Content-Type", "application/json")
              ->SetBody(requestBody.str().c_str(), requestBody.str().size(),
                        true);
          _client->SendRequest(request, pendingRequests);
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