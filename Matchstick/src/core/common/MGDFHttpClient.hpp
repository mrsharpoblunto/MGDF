#pragma once

#include <json/json.h>

#include <deque>
#include <functional>
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

class HttpClient;

struct HttpResponse {
  int Code = 0;
  std::string Body;
  std::unordered_map<std::string, std::string> Headers;
};

class HttpRequest {
  friend class HttpClient;

 public:
  virtual ~HttpRequest();

  HttpRequest *SetMethod(const std::string &method);
  HttpRequest *SetHeader(const std::string &header, const std::string &value);
  HttpRequest *SetBody(const char *body, size_t bodyLength,
                       bool compress = false);
  void Send();
  bool GetHeader(const std::string &header, std::string &value) const;
  bool GetResponse(std::shared_ptr<HttpResponse> &response) const;
  bool GetLastError(std::string &error) const;
  HttpRequestState GetState() const;

 private:
  HttpRequest(const std::string &url);

  mutable std::mutex _mutex;
  std::string _url;
  std::string _method;
  std::string _body;
  std::string _error;
  HttpRequestState _state;
  std::unordered_map<std::string, std::string> _headers;
  std::shared_ptr<HttpResponse> _response;
};

struct HttpOrigin;

struct HttpConnection {
  std::shared_ptr<HttpRequest> Request;
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
  std::deque<std::shared_ptr<HttpRequest>> PendingRequests;
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

  std::shared_ptr<HttpRequest> GetRequest(const std::string &url);

 private:
  static void HandleResponse(struct mg_connection *c, int ev, void *ev_data,
                             void *fn_data);
  static bool MakeRequestWithConnection(HttpConnection *connection);
  static void GetResponseFromConnection(HttpConnection *conn,
                                        mg_http_message *hm);
  static void ParseKeepAliveHeader(const mg_str *header, int &timeout,
                                   int &max);
  static void LoadCerts(
      std::function<void(const std::string &, const std::string &)> insert);

  std::deque<std::shared_ptr<HttpRequest>> _pendingRequests;
  std::unordered_map<std::string, HttpOrigin> _origins;
  std::mutex _mutex;
  struct mg_fs _fs;
  std::thread _pollThread;
  bool _running;
  HttpClientOptions _options;
};

// mongoose has a virtual filsystem, so this is an in-memory
// implementation to populate with SSL certs as needed
class MemFS {
 public:
  static void Ensure(
      const std::string &file,
      std::function<
          void(std::function<void(const std::string &, const std::string &)>)>);
  static void InitMGFS(mg_fs &fs);

 private:
  struct FD {
    std::string &Content;
    size_t Offset;
  };
  static std::unordered_map<std::string, std::string> _content;
  static std::mutex _mutex;

  static int st(const char *path, size_t *size, time_t *mtime);
  static void *op(const char *path, int flags);
  static void cl(void *fd);
  static size_t rd(void *fd, void *buf, size_t len);  // Read file
};

}  // namespace core
}  // namespace MGDF
