#pragma once

#include <deque>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "../MGDFNetworkManagerComponent.hpp"
#include "mongoose.h"

namespace MGDF {
namespace core {
namespace network {
namespace mongoose {

class MongooseNetworkManagerComponent;
class HttpServer;

class HttpServerRequest
    : public std::enable_shared_from_this<HttpServerRequest>,
      public IHttpServerRequest {
 public:
  HttpServerRequest(mg_connection *connection, HttpMessage &request,
                    std::shared_ptr<HttpServer> server);
  virtual ~HttpServerRequest();

  bool GetRequestHeader(const std::string &name,
                        std::string &value) const final;
  bool HasRequestHeader(const std::string &name) const final;
  const std::string &GetRequestHeader(const std::string &name) const final;
  const std::string &GetRequestMethod() const final;
  const std::string &GetRequestUrl() const final;
  const std::string &GetRequestBody() const final;

  IHttpServerRequest *SetResponseCode(int code) final;
  IHttpServerRequest *SetResponseHeader(const std::string &name,
                                        const std::string &value) final;
  IHttpServerRequest *SetResponseBody(const char *body,
                                      size_t bodyLength) final;
  void SendResponse() final;

 private:
  mg_connection *_connection;
  std::shared_ptr<HttpServer> _server;
  HttpMessage _request;
  HttpMessage _response;
  bool _sent;
};

struct HttpOrigin;
class HttpClientPendingRequest;

struct HttpConnection {
  std::shared_ptr<HttpClientPendingRequest> Request;
  size_t KeepAlive;
  size_t KeepAliveDuration;
  size_t ConnectTimeout;
  const size_t ConnectTimeoutDuration;
  HttpOrigin *Origin;
  mg_connection *Connection;
};

struct HttpOrigin {
  std::function<void(mg_connection *)> TLSInit;
  size_t ConnectionLimit = 0;
  std::string Host;
  std::mutex Mutex;
  std::unordered_map<mg_connection *, std::shared_ptr<HttpConnection>>
      Connections;
  std::deque<std::shared_ptr<HttpClientPendingRequest>> PendingRequests;
};

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
        _state(HttpRequestState::Requesting) {}
  virtual ~HttpClientPendingRequest() {}

  void CancelRequest() final;
  HttpRequestState GetRequestState() const final;
  bool GetResponse(std::shared_ptr<HttpMessage> &response) final;

  void SetResponse(HttpRequestState state, HttpMessage &response);
  bool SendRequest(HttpConnection &conn);
  void ReceiveResponse(HttpConnection &conn, mg_http_message *hm);
  const std::string &GetUrl() const { return _request.Url; }

 private:
  mutable std::mutex _mutex;
  HttpRequestState _state;
  HttpMessage _request;
  std::shared_ptr<HttpMessage> _response;
  std::function<void(std::shared_ptr<HttpMessage> &response)> _responseHandler;
};

class HttpClientRequest
    : public std::enable_shared_from_this<HttpClientRequest>,
      public IHttpClientRequest {
 public:
  virtual ~HttpClientRequest() {}
  HttpClientRequest(const std::string &url,
                    std::shared_ptr<MongooseNetworkManagerComponent> manager);

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
  const std::shared_ptr<MongooseNetworkManagerComponent> _manager;
  HttpMessage _request;
};

class WebSocketBase : public IWebSocket {
 public:
  WebSocketBase(MGDFWebSocketConnectionState state) : _state(state) {}
  virtual ~WebSocketBase() {}
  IWebSocket *Send(const std::vector<char> &data, bool binary) final;
  IWebSocket *Send(void *data, size_t dataLength, bool binary) final;
  IWebSocket *OnReceive(
      std::function<void(std::span<const char> &message, bool binary)> handler)
      final;
  MGDFWebSocketConnectionState GetConnectionState(
      std::string &lastError) const final;

 protected:
  std::function<void(std::span<const char> &message, bool binary)> _inHandler;
  std::vector<WebSocketMessage> _out;
  std::string _lastError;
  MGDFWebSocketConnectionState _state;
  mutable std::mutex _mutex;
};

class WebSocket : public std::enable_shared_from_this<WebSocket>,
                  public WebSocketBase {
 public:
  virtual ~WebSocket();
  WebSocket(const std::string &url,
            std::shared_ptr<MongooseNetworkManagerComponent> manager,
            size_t reconnectInterval);

  void Connect(mg_mgr &mgr);

 private:
  static void HandleEvents(mg_connection *c, int ev, void *ev_data,
                           void *fn_data);

  bool _usesTLS;
  std::string _host;
  const size_t _reconnectInterval;
  const std::string _url;
  const std::shared_ptr<MongooseNetworkManagerComponent> _manager;

  bool _forceClose;
  bool _closing;
  std::condition_variable _cv;
};

class ServerWebSocket : public std::enable_shared_from_this<ServerWebSocket>,
                        public WebSocketBase {
 public:
  virtual ~ServerWebSocket();
  ServerWebSocket(mg_connection *connection,
                  std::shared_ptr<HttpServer> server);

  void ReceiveMessage(std::span<const char> &message, bool binary);
  void SendMessages();
  void ForceClose();
  void SetLastError(const std::string_view &error) { _lastError = error; }

 private:
  mg_connection *_connection;
  std::shared_ptr<HttpServer> _server;
  std::condition_variable _cv;
};

class HttpServer : public std::enable_shared_from_this<HttpServer>,
                   public IHttpServer {
 public:
  virtual ~HttpServer();
  HttpServer(const std::string &webSocketPath,
             std::shared_ptr<MongooseNetworkManagerComponent> manager);

  IHttpServer *OnHttpRequest(
      std::function<void(std::shared_ptr<IHttpServerRequest> request)> handler)
      final;
  IHttpServer *OnWebSocketRequest(
      std::function<void(std::shared_ptr<IWebSocket> socket)> handler) final;
  bool Listening() const final { return _listening.load(); }

  void Listen(mg_mgr &mgr, uint32_t port);

  void SendResponse(mg_connection *conn, HttpMessage &response);
  static void HandleEvents(mg_connection *c, int ev, void *ev_data,
                           void *fn_data);

  void QueueMessage(std::shared_ptr<ServerWebSocket> socket);
  void QueueClose(mg_connection *c);

 private:
  std::mutex _stateMutex;
  std::condition_variable _cv;
  bool _closed;
  std::atomic_bool _listening;
  std::atomic_bool _closing;
  const std::string _webSocketPath;
  std::function<void(std::shared_ptr<IHttpServerRequest> request)>
      _requestHandler;
  std::function<void(std::shared_ptr<IWebSocket> socket)> _webSocketHandler;
  std::shared_ptr<MongooseNetworkManagerComponent> _manager;

  std::mutex _httpResponseMutex;
  std::unordered_map<mg_connection *, HttpMessage> _pendingServerResponses;

  std::mutex _webSocketMutex;
  std::unordered_map<mg_connection *, std::weak_ptr<ServerWebSocket>>
      _webSockets;
  std::unordered_set<std::shared_ptr<ServerWebSocket>>
      _pendingWebSocketMessages;
  std::unordered_set<mg_connection *> _pendingWebSocketClosures;
};

class MongooseNetworkManagerComponent
    : public std::enable_shared_from_this<MongooseNetworkManagerComponent>,
      public INetworkManagerComponent {
 public:
  MongooseNetworkManagerComponent(const NetworkManagerOptions &options);
  virtual ~MongooseNetworkManagerComponent();
  std::unique_ptr<IHttpClientRequest> CreateHttpRequest(
      const std::string &url) final;
  std::shared_ptr<IHttpServer> CreateHttpServer(
      uint32_t port, const std::string &socketPath) final;
  std::shared_ptr<IWebSocket> CreateWebSocket(const std::string &url) final;

  std::shared_ptr<HttpClientPendingRequest> SendRequest(
      HttpMessage &request,
      std::function<void(std::shared_ptr<HttpMessage> &response)> handler);

  void TLSInit(mg_connection *conn, const std::string &host);
  void ReconnectWebSocket(std::weak_ptr<WebSocket> socket, size_t when);

 private:
  void Poll();
  static void HandleEvents(mg_connection *c, int ev, void *ev_data,
                           void *fn_data);

  mg_mgr _mgr;
  mg_fs _fs;
  std::thread _pollThread;
  NetworkManagerOptions _options;
  bool _running;

  std::mutex _originMutex;
  std::unordered_map<std::string, std::shared_ptr<HttpOrigin>> _origins;

  std::mutex _webSocketMutex;
  std::list<std::pair<std::weak_ptr<WebSocket>, size_t>> _pendingWebSockets;

  std::mutex _serverMutex;
  std::vector<std::pair<std::weak_ptr<HttpServer>, uint32_t>> _pendingServers;
};

void ParseKeepAliveHeader(const mg_str *header, int &timeout, int &max);

}  // namespace mongoose
}  // namespace network
}  // namespace core
}  // namespace MGDF
