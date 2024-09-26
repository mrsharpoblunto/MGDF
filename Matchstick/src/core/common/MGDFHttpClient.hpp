#pragma once

#include <deque>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "mongoose.h"

namespace MGDF {
namespace core {

enum class HttpRequestState {
  Preparing,
  Pending,
  Requesting,
  Cancelled,
  Error,
  Complete,
};

struct HttpResponse {
  int Code = 0;
  std::vector<char> Body;
  std::string Error;
  std::unordered_map<std::string, std::string> Headers;
};

class HttpClient;
class HttpRequestGroup;

class HttpRequest {
  friend class HttpClient;
  friend class HttpRequestGroup;

 public:
  virtual ~HttpRequest();
  HttpRequest(const std::string &url);

  HttpRequest *SetMethod(const std::string &method);
  HttpRequest *SetHeader(const std::string &header, const std::string &value);
  HttpRequest *SetBody(const char *body, size_t bodyLength,
                       bool compress = false);
  bool GetHeader(const std::string &header, std::string &value) const;
  bool GetResponse(std::shared_ptr<HttpResponse> &response) const;
  HttpRequestState GetState() const;

  void Cancel();

 private:
  mutable std::mutex _mutex;
  std::string _url;
  std::string _method;
  std::string _body;
  HttpRequestState _state;
  std::unordered_map<std::string, std::string> _headers;
  std::shared_ptr<HttpResponse> _response;
};

class HttpRequestGroup {
  friend class HttpClient;

 public:
  virtual ~HttpRequestGroup() {}
  std::shared_ptr<HttpRequest> GetResponse(
      std::shared_ptr<HttpResponse> &response);

  size_t Size() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _requests.size();
  }

 private:
  std::deque<std::shared_ptr<HttpRequest>> _requests;
  mutable std::mutex _mutex;
};

struct HttpOrigin;

struct HttpConnection {
  std::shared_ptr<HttpRequest> Request;
  std::shared_ptr<HttpRequestGroup> Group;
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
  std::deque<std::pair<std::shared_ptr<HttpRequest>,
                       std::shared_ptr<HttpRequestGroup>>>
      PendingRequests;
};

struct HttpClientOptions {
  size_t OriginConnectionLimit;
  size_t ConnectionTimeout;
  size_t KeepAlive;
};

class HttpClient {
 public:
  static HttpClientOptions DEFAULT_OPTIONS;

  HttpClient(HttpClientOptions &options = DEFAULT_OPTIONS);
  virtual ~HttpClient();

  void SendRequest(std::shared_ptr<HttpRequest> request,
                   std::shared_ptr<HttpRequestGroup> group =
                       std::shared_ptr<HttpRequestGroup>());

 private:
  static void HandleResponse(struct mg_connection *c, int ev, void *ev_data,
                             void *fn_data);
  static bool MakeRequestWithConnection(HttpConnection *connection);
  static void GetResponseFromConnection(HttpConnection *conn,
                                        mg_http_message *hm);
  static void ParseKeepAliveHeader(const mg_str *header, int &timeout,
                                   int &max);

  std::deque<std::pair<std::shared_ptr<HttpRequest>,
                       std::shared_ptr<HttpRequestGroup>>>
      _pendingRequests;
  std::unordered_map<std::string, HttpOrigin> _origins;
  std::mutex _mutex;
  struct mg_fs _fs;
  std::thread _pollThread;
  bool _running;
  HttpClientOptions _options;
};

}  // namespace core
}  // namespace MGDF
