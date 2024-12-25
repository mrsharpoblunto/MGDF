#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "MGDFHttpCommon.hpp"
#include "mongoose.h"

namespace MGDF {
namespace core {

typedef HttpMessageBase HttpClientResponse;

enum class HttpRequestState {
  Preparing,
  Pending,
  Requesting,
  Cancelled,
  Error,
  Complete,
};

class HttpClient;
class HttpClientRequestGroup;

// Need common Request and ServerRequest and ClientRequest classes
// serverrequest allows writing to the response and clientrequest allows writing
// to the request
class HttpClientRequest {
  friend class HttpClient;
  friend class HttpClientRequestGroup;

 public:
  virtual ~HttpClientRequest();
  HttpClientRequest(const std::string &url);

  HttpClientRequest *SetMethod(const std::string &method);
  HttpClientRequest *SetHeader(const std::string &header,
                               const std::string &value);
  HttpClientRequest *SetBody(const char *body, size_t bodyLength,
                             bool compress = false);
  bool GetHeader(const std::string &header, std::string &value) const;
  bool GetResponse(std::shared_ptr<HttpClientResponse> &response) const;
  HttpRequestState GetState() const;

  void Cancel();

 private:
  mutable std::mutex _mutex;
  std::string _url;
  HttpRequestState _state;
  HttpMessageBase _request;
  std::shared_ptr<HttpMessageBase> _response;
};

class HttpClientRequestGroup {
  friend class HttpClient;

 public:
  virtual ~HttpClientRequestGroup() {}
  std::shared_ptr<HttpClientRequest> GetResponse(
      std::shared_ptr<HttpClientResponse> &response);

  size_t Size() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _requests.size();
  }

 private:
  std::deque<std::shared_ptr<HttpClientRequest>> _requests;
  mutable std::mutex _mutex;
};

struct HttpOrigin;

struct HttpConnection {
  std::shared_ptr<HttpClientRequest> Request;
  std::shared_ptr<HttpClientRequestGroup> Group;
  size_t KeepAlive;
  size_t KeepAliveDuration;
  size_t ConnectTimeout;
  const size_t ConnectTimeoutDuration;
  HttpOrigin *Origin;
  mg_connection *Connection;
};

struct HttpOrigin {
  bool UsesTLS;
  size_t ConnectionLimit;
  std::string Host;
  mg_fs *MemFS;
  std::unordered_map<mg_connection *, HttpConnection *> IdleConnections;
  std::unordered_map<mg_connection *, HttpConnection> Connections;
  std::deque<std::pair<std::shared_ptr<HttpClientRequest>,
                       std::shared_ptr<HttpClientRequestGroup>>>
      PendingRequests;
};

struct HttpClientOptions {
  size_t OriginConnectionLimit;
  size_t ConnectionTimeout;
  size_t KeepAlive;
};

class HttpClient: public INetworkEventListener {
 public:
  static HttpClientOptions DEFAULT_OPTIONS;

  HttpClient(std::shared_ptr<NetworkEventLoop> eventLoop, HttpClientOptions &options = DEFAULT_OPTIONS);
  virtual ~HttpClient();

  void SendRequest(std::shared_ptr<HttpClientRequest> request,
                   std::shared_ptr<HttpClientRequestGroup> group =
                       std::shared_ptr<HttpClientRequestGroup>());

void OnPoll(mg_mgr &mgr, mg_fs &fs) final;

 private:
  static void HandleResponse(struct mg_connection *c, int ev, void *ev_data,
                             void *fn_data);
  static bool MakeRequestWithConnection(HttpConnection *connection);
  static void GetResponseFromConnection(HttpConnection *conn,
                                        mg_http_message *hm);
  static void ParseKeepAliveHeader(const mg_str *header, int &timeout,
                                   int &max);

  std::deque<std::pair<std::shared_ptr<HttpClientRequest>,
                       std::shared_ptr<HttpClientRequestGroup>>>
      _pendingRequests;
  std::unordered_map<std::string, HttpOrigin> _origins;
  std::mutex _mutex;
  std::shared_ptr<NetworkEventLoop> _eventLoop;
  HttpClientOptions _options;
};

}  // namespace core
}  // namespace MGDF
