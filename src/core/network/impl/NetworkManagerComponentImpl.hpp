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

struct WebSocketTransport {
  std::function<CURL *()> Create = curl_easy_init;
  std::function<void(CURL *)> Release = curl_easy_cleanup;
};

class WebSocketConnection
    : public std::enable_shared_from_this<WebSocketConnection>,
      public WebSocketBase {
 public:
  WebSocketConnection(const std::string &url, NetworkManagerComponent *manager,
                      size_t reconnectInterval, WebSocketTransport transport);
  virtual ~WebSocketConnection();

  IWebSocket *Send(const std::vector<char> &data, bool binary) final;
  IWebSocket *Send(void *data, size_t dataLength, bool binary) final;

  enum class PumpResult {
    Active,
    Closed,
  };

  void RequestClose();
  bool Closing() const;

  // The manager poll thread owns transport access.
  bool Connect(CURLM *multi);
  void CompleteConnection(CURLM *multi, CURLcode result);
  PumpResult Pump(CURLM *multi);
  void Shutdown(CURLM *multi);
  CURL *Connection() const { return _curl; }

 private:
  void Fail(CURLM *multi, const std::string &error);

  const std::string _url;
  const size_t _reconnectInterval;
  NetworkManagerComponent *const _manager;
  const WebSocketTransport _transport;
  bool _forceClose;
  bool _closing;

  // poll thread only
  CURL *_curl;
  bool _connecting;
  char _error[CURL_ERROR_SIZE];
  std::deque<WebSocketMessage> _sendQueue;
  size_t _sendOffset;
  std::vector<char> _recvBuffer;
  bool _recvBinary;
};

class WebSocketClient : public IWebSocket {
 public:
  WebSocketClient(std::shared_ptr<WebSocketConnection> connection,
                  std::shared_ptr<NetworkManagerComponent> manager)
      : _connection(std::move(connection)), _manager(std::move(manager)) {}
  ~WebSocketClient() override;
  IWebSocket *Send(const std::vector<char> &data, bool binary) final;
  IWebSocket *Send(void *data, size_t dataLength, bool binary) final;
  IWebSocket *OnReceive(
      std::function<void(std::span<const char> &, bool)> handler) final;
  MGDFWebSocketConnectionState GetConnectionState(
      std::string &lastError) const final;

 private:
  const std::shared_ptr<WebSocketConnection> _connection;
  const std::shared_ptr<NetworkManagerComponent> _manager;
};

class NetworkManagerComponent
    : public std::enable_shared_from_this<NetworkManagerComponent>,
      public INetworkManagerComponent {
 public:
  NetworkManagerComponent(const NetworkManagerOptions &options,
                          WebSocketTransport transport = {});
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

  void ScheduleWebSocketConnect(std::shared_ptr<WebSocketConnection> socket,
                                size_t when);
  void Wake();

  static size_t NowMs();

 private:
  void Poll();

  CURLM *_multi;
  std::thread _pollThread;
  NetworkManagerOptions _options;
  std::atomic_bool _running;
  const WebSocketTransport _transport;
  std::mutex _stopMutex;
  std::thread::id _pollThreadId;

  std::mutex _requestMutex;
  std::deque<std::shared_ptr<HttpClientPendingRequest>> _newRequests;

  std::mutex _webSocketMutex;
  std::list<std::pair<std::shared_ptr<WebSocketConnection>, size_t>>
      _pendingWebSockets;

  // Poll owns connection state until transport cleanup is complete.
  std::unordered_map<CURL *, std::shared_ptr<HttpClientPendingRequest>>
      _activeRequests;
  std::vector<std::shared_ptr<WebSocketConnection>> _activeWebSockets;
};

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
