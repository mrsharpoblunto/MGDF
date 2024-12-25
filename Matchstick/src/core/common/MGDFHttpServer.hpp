#pragma once

#include <string>
#include <thread>
#include <unordered_set>

#include "MGDFHttpCommon.hpp"
#include "MGDFWebSocket.hpp"
#include "mongoose.h"

namespace MGDF {
namespace core {

class HttpServer;

class WebSocketServerConnection : public WebSocketConnectionBase {
  friend class HttpServer;

 public:
  WebSocketServerConnection(mg_connection *c, HttpServer *server);
  virtual ~WebSocketServerConnection();

 private:
  HttpServer *_server;
  std::shared_ptr<std::mutex> _serverMutex;
};

class HttpServerRequest {
  friend class HttpServer;

 public:
  HttpServerRequest(mg_connection *c, struct mg_http_message *m,
                    HttpServer *server);
  virtual ~HttpServerRequest();

  bool GetRequestHeader(const std::string &name, std::string &value) const;
  bool HasRequestHeader(const std::string &name) const;
  const std::string &GetRequestHeader(const std::string &name) const;
  const std::string &GetRequestMethod() const;
  const std::string &GetRequestUrl() const;
  const std::string &GetRequestBody() const;

  HttpServerRequest *SetResponseCode(int code);
  HttpServerRequest *SetResponseHeader(const std::string &name,
                                       const std::string &value);
  HttpServerRequest *SetResponseMethod(const std::string &method);
  HttpServerRequest *SetResponseBody(const char *body, UINT64 bodyLength);
  void SendResponse();

 private:
  std::shared_ptr<HttpMessageBase> _request;
  std::string _url;
  HttpMessageBase _response;
  mg_connection *_conn;
  HttpServer *_server;
  std::shared_ptr<std::mutex> _serverMutex;
};

class HttpServer : public INetworkEventListener {
  friend class HttpServerRequest;
  friend class WebSocketServerConnection;

 public:
  HttpServer(std::shared_ptr<NetworkEventLoop> &eventLoop);
  virtual ~HttpServer();

  void Listen(const std::string &port, const std::string &socketPath = "");
  bool Listening() const { return _conn != nullptr; }

  virtual void OnRequest(std::shared_ptr<HttpServerRequest> &request) = 0;
  virtual void OnSocketRequest(
      std::shared_ptr<WebSocketServerConnection> &socket) = 0;

  void OnPoll(mg_mgr &mgr, mg_fs &fs) final;

 private:
  static void HandleRequest(mg_connection *c, int ev, void *ev_data,
                            void *fn_data);
  struct mg_connection *_conn;
  std::string _socketPath;

  std::shared_ptr<NetworkEventLoop> _eventLoop;
  std::string _pendingListen;
  std::shared_ptr<std::mutex> _mutex;
  std::unordered_map<HttpServerRequest *, std::weak_ptr<HttpServerRequest>>
      _pendingResponses;
  std::unordered_map<mg_connection *, std::weak_ptr<WebSocketServerConnection>>
      _sockets;
  std::vector<std::pair<mg_connection *, HttpMessageBase>> _responses;
};

}  // namespace core
}  // namespace MGDF
