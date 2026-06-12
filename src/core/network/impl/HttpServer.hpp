#pragma once

#include <civetweb.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "../MGDFHttpServer.hpp"
#include "HttpCommon.hpp"
#include "WebSocketBase.hpp"

namespace MGDF {
namespace core {
namespace network {
namespace impl {

class HttpServer;

// shared between the civetweb worker thread (which blocks awaiting the
// response) and the consumer that eventually calls SendResponse
struct PendingHttpResponse {
  std::mutex Mutex;
  std::condition_variable Cv;
  bool Ready = false;
  HttpMessage Response;
};

class HttpServerRequest : public IHttpServerRequest {
 public:
  HttpServerRequest(HttpMessage &request,
                    std::shared_ptr<PendingHttpResponse> pending);
  virtual ~HttpServerRequest();

  bool GetRequestHeader(const std::string &name,
                        std::string &value) const final;
  bool HasRequestHeader(const std::string &name) const final;
  const std::string &GetRequestHeader(const std::string &name) const final;
  const std::string &GetRequestMethod() const final;
  const std::string &GetRequestPath() const final;
  const std::string &GetRequestBody() const final;

  IHttpServerRequest *SetResponseCode(int code) final;
  IHttpServerRequest *SetResponseHeader(const std::string &name,
                                        const std::string &value) final;
  IHttpServerRequest *SetResponseBody(const char *body,
                                      size_t bodyLength) final;
  void SendResponse() final;

 private:
  std::shared_ptr<PendingHttpResponse> _pending;
  HttpMessage _request;
  HttpMessage _response;
  bool _sent;
};

class ServerWebSocket : public std::enable_shared_from_this<ServerWebSocket>,
                        public WebSocketBase {
 public:
  ServerWebSocket(mg_connection *connection,
                  std::shared_ptr<HttpServer> server);
  virtual ~ServerWebSocket();

  IWebSocket *Send(const std::vector<char> &data, bool binary) final;
  IWebSocket *Send(void *data, size_t dataLength, bool binary) final;

  // called by the civetweb worker thread for this connection
  void ReceiveFrame(int bits, const char *data, size_t dataLength);
  void ForceClose();

 private:
  void SendFrame(const void *data, size_t dataLength, bool binary);
  void Deliver(std::span<const char> &message, bool binary);

  mg_connection *_connection;
  std::shared_ptr<HttpServer> _server;
  // fragment reassembly state, only touched by this connections worker thread
  std::vector<char> _fragments;
  bool _fragmentBinary;
};

class HttpServer : public std::enable_shared_from_this<HttpServer>,
                   public IHttpServer {
 public:
  HttpServer(const std::string &webSocketPath);
  virtual ~HttpServer();

  IHttpServer *OnHttpRequest(
      std::function<void(std::shared_ptr<IHttpServerRequest> request)> handler)
      final;
  IHttpServer *OnWebSocketRequest(
      std::function<void(std::shared_ptr<IWebSocket> socket)> handler) final;
  bool Listening() const final { return _listening.load(); }

  void Listen(uint32_t port);

  // removes the websocket from the connection map and, if the connection
  // is still open, sends a close frame. Used when a consumer discards a
  // ServerWebSocket while the underlying connection is still connected
  void CloseWebSocket(mg_connection *connection);

 private:
  static int HandleRequest(mg_connection *conn, void *cbdata);
  static int HandleWebSocketConnect(const mg_connection *conn, void *cbdata);
  static void HandleWebSocketReady(mg_connection *conn, void *cbdata);
  static int HandleWebSocketData(mg_connection *conn, int bits, char *data,
                                 size_t dataLength, void *cbdata);
  static void HandleWebSocketClose(const mg_connection *conn, void *cbdata);
  static int LogMessage(const mg_connection *conn, const char *message);

  static void SendHttpResponse(mg_connection *conn, const HttpMessage &m);

  mg_context *_ctx;
  std::atomic_bool _listening;
  std::atomic_bool _closing;
  const std::string _webSocketPath;

  std::mutex _mutex;
  std::function<void(std::shared_ptr<IHttpServerRequest> request)>
      _requestHandler;
  std::function<void(std::shared_ptr<IWebSocket> socket)> _webSocketHandler;
  std::unordered_set<std::shared_ptr<PendingHttpResponse>> _pendingResponses;
  std::unordered_map<const mg_connection *, std::weak_ptr<ServerWebSocket>>
      _webSockets;
};

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
