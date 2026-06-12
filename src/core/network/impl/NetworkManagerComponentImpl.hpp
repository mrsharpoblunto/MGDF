#pragma once

#include <curl/curl.h>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <list>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <unordered_map>

#include "../MGDFNetworkManagerComponent.hpp"
#include "HttpServer.hpp"
#include "WebSocketBase.hpp"

namespace MGDF {
namespace core {
namespace network {
namespace impl {

class NetworkManagerComponent;

class HttpClientPendingRequest
    : public std::enable_shared_from_this<HttpClientPendingRequest>,
      public IHttpClientPendingRequest {
 public:
  HttpClientPendingRequest(
      HttpMessage &request,
      std::function<void(std::shared_ptr<HttpMessage> &response)>
          responseHandler)
      : _request(std::move(request)),
        _responseHandler(responseHandler),
        _state(HttpRequestState::Requesting),
        _curl(nullptr),
        _headers(nullptr) {
    _error[0] = '\0';
  }
  virtual ~HttpClientPendingRequest();

  void CancelRequest() final;
  HttpRequestState GetRequestState() const final;
  bool GetResponse(std::shared_ptr<HttpMessage> &response) final;

  void SetResponse(HttpRequestState state, HttpMessage &response);

  // the following members are only used by the manager poll thread
  bool CreateConnection(CURLM *multi, const NetworkManagerOptions &options);
  void CompleteConnection(CURLcode result);
  void ReleaseConnection(CURLM *multi);
  CURL *Connection() const { return _curl; }

 private:
  static size_t WriteResponseBody(char *data, size_t size, size_t members,
                                  void *context);
  static size_t WriteResponseHeader(char *data, size_t size, size_t members,
                                    void *context);

  mutable std::shared_mutex _mutex;
  HttpRequestState _state;
  HttpMessage _request;
  std::shared_ptr<HttpMessage> _response;
  std::function<void(std::shared_ptr<HttpMessage> &response)> _responseHandler;

  // poll thread only
  HttpMessage _working;
  std::vector<char> _requestBody;
  CURL *_curl;
  curl_slist *_headers;
  char _error[CURL_ERROR_SIZE];
};

class HttpClientRequest
    : public std::enable_shared_from_this<HttpClientRequest>,
      public IHttpClientRequest {
 public:
  virtual ~HttpClientRequest() {}
  HttpClientRequest(const std::string &url,
                    std::shared_ptr<NetworkManagerComponent> manager);

  IHttpClientRequest *SetRequestMethod(const std::string &method) final;
  IHttpClientRequest *SetRequestHeader(const std::string &header,
                                       const std::string &value) final;
  IHttpClientRequest *SetRequestBody(const char *body, size_t bodyLength,
                                     bool compress) final;
  bool GetRequestHeader(const std::string &header,
                        std::string &value) const final;

  std::shared_ptr<IHttpClientPendingRequest> SendRequest(
      std::function<void(std::shared_ptr<HttpMessage> &response)> handler)
      final;

 private:
  const std::shared_ptr<NetworkManagerComponent> _manager;
  HttpMessage _request;
};

class WebSocketClient : public std::enable_shared_from_this<WebSocketClient>,
                        public WebSocketBase {
 public:
  WebSocketClient(const std::string &url,
                  std::shared_ptr<NetworkManagerComponent> manager,
                  size_t reconnectInterval);
  virtual ~WebSocketClient();

  IWebSocket *Send(const std::vector<char> &data, bool binary) final;
  IWebSocket *Send(void *data, size_t dataLength, bool binary) final;

  enum class PumpResult {
    Active,
    Closed,
  };

  // the following members are only used by the manager poll thread
  bool Connect(CURLM *multi);
  void CompleteConnection(CURLM *multi, CURLcode result);
  PumpResult Pump(CURLM *multi);
  void Shutdown(CURLM *multi);
  CURL *Connection() const { return _curl; }

 private:
  void Fail(CURLM *multi, const std::string &error);

  const std::string _url;
  const size_t _reconnectInterval;
  const std::shared_ptr<NetworkManagerComponent> _manager;
  bool _forceClose;
  bool _closing;
  std::condition_variable_any _cv;

  // poll thread only
  CURL *_curl;
  bool _connecting;
  char _error[CURL_ERROR_SIZE];
  std::deque<WebSocketMessage> _sendQueue;
  size_t _sendOffset;
  std::vector<char> _recvBuffer;
  bool _recvBinary;
};

class NetworkManagerComponent
    : public std::enable_shared_from_this<NetworkManagerComponent>,
      public INetworkManagerComponent {
 public:
  NetworkManagerComponent(const NetworkManagerOptions &options);
  virtual ~NetworkManagerComponent();

  std::unique_ptr<IHttpClientRequest> CreateHttpRequest(
      const std::string &url) final;
  std::shared_ptr<IHttpServer> CreateHttpServer(
      uint32_t port, const std::string &socketPath) final;
  std::shared_ptr<IWebSocket> CreateWebSocket(const std::string &url) final;

  void Stop() final;

  std::shared_ptr<HttpClientPendingRequest> SendRequest(
      HttpMessage &request,
      std::function<void(std::shared_ptr<HttpMessage> &response)> handler);

  void ScheduleWebSocketConnect(std::weak_ptr<WebSocketClient> socket,
                                size_t when);
  void Wake();

  static size_t NowMs();

 private:
  void Poll();

  CURLM *_multi;
  std::thread _pollThread;
  NetworkManagerOptions _options;
  std::atomic_bool _running;

  std::mutex _requestMutex;
  std::deque<std::shared_ptr<HttpClientPendingRequest>> _newRequests;

  std::mutex _webSocketMutex;
  std::list<std::pair<std::weak_ptr<WebSocketClient>, size_t>>
      _pendingWebSockets;

  // poll thread only. Active websockets are tracked by raw pointer - this
  // is safe as a WebSocketClients destructor blocks until the poll thread
  // has acknowledged the close and stopped referencing it
  std::unordered_map<CURL *, std::shared_ptr<HttpClientPendingRequest>>
      _activeRequests;
  std::vector<WebSocketClient *> _activeWebSockets;
};

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
