#include "stdafx.h"

#include "MongooseNetworkManagerComponentImpl.hpp"

#include <algorithm>
#include <sstream>

#include "CertificateManager.hpp"
#include "MongooseHttpCommon.hpp"
#include "MongooseMemFS.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

static const std::string S_HTTP("http://");
static const std::string S_HTTPS("https://");
static const std::string S_GET("GET");
static const std::string S_GZIP("gzip");
static const std::string S_EMPTY("");
static const std::string S_IDENTITY("identity");
static const std::string S_CONTENT_ENCODING("Content-Encoding");
static const std::string S_ACCEPT_ENCODING("Accept-Encoding");
static const std::string S_TIMEOUT("timeout");
static const std::string S_MAX("max");
static const std::string S_CANCELLED("Cancelled");
static const std::string S_CONNECTION("Connection");
static const std::string S_KEEP_ALIVE("Keep-Alive");
static const std::string S_CLOSE("close");

namespace MGDF {
namespace core {
namespace network {
namespace mongoose {

HttpServerRequest::HttpServerRequest(mg_connection *connection,
                                     HttpMessage &request,
                                     std::shared_ptr<HttpServer> server)
    : _sent(false),
      _connection(connection),
      _request(std::move(request)),
      _server(server) {
  _response.Method = _request.Method;
  _response.Url = _request.Url;
  std::string encoding(S_IDENTITY);
  std::string acceptEncoding;
  if (GetRequestHeader(S_ACCEPT_ENCODING, acceptEncoding) &&
      acceptEncoding.find(S_GZIP) != std::string_view::npos) {
    encoding = S_GZIP;
  }
  _response.Headers.insert(std::make_pair(S_CONTENT_ENCODING, encoding));
}

HttpServerRequest::~HttpServerRequest() {
  if (!_sent) {
    _response.Code = 500;
    _response.Body = "Internal Server Error";
    SendResponse();
  }
}

bool HttpServerRequest::GetRequestHeader(const std::string &name,
                                         std::string &value) const {
  auto found = _request.Headers.find(name);
  if (found != _request.Headers.end()) {
    value = found->second;
    return true;
  }
  return false;
}

bool HttpServerRequest::HasRequestHeader(const std::string &name) const {
  return _request.Headers.find(name) != _request.Headers.end();
}

const std::string &HttpServerRequest::GetRequestHeader(
    const std::string &name) const {
  auto found = _request.Headers.find(name);
  if (found != _request.Headers.end()) {
    return found->second;
  }
  return S_EMPTY;
}

const std::string &HttpServerRequest::GetRequestMethod() const {
  return _request.Method;
}

const std::string &HttpServerRequest::GetRequestPath() const {
  return _request.Url;
}

const std::string &HttpServerRequest::GetRequestBody() const {
  return _request.Body;
}

IHttpServerRequest *HttpServerRequest::SetResponseCode(int code) {
  _response.Code = code;
  return this;
}

IHttpServerRequest *HttpServerRequest::SetResponseHeader(
    const std::string &name, const std::string &value) {
  _response.Headers.insert(std::make_pair(name, value));
  return this;
}

IHttpServerRequest *HttpServerRequest::SetResponseBody(const char *body,
                                                       size_t bodyLength) {
  _response.Body.assign(body, bodyLength);
  return this;
}

void HttpServerRequest::SendResponse() {
  if (!_sent) {
    _server->SendResponse(_connection, _response);
    _sent = true;
  }
}

void HttpClientPendingRequest::CancelRequest() {
  HttpMessage response = {.Code = -1, .Error = S_CANCELLED};
  SetResponse(HttpRequestState::Cancelled, response);
}

HttpRequestState HttpClientPendingRequest::GetRequestState() const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  return _state;
}

bool HttpClientPendingRequest::GetResponse(
    std::shared_ptr<HttpMessage> &response) {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  if (_state == HttpRequestState::Complete ||
      _state == HttpRequestState::Error ||
      _state == HttpRequestState::Cancelled) {
    response = _response;
    return true;
  }
  return false;
}

void HttpClientPendingRequest::SetResponse(HttpRequestState state,
                                           HttpMessage &response) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  if (_state == HttpRequestState::Requesting) {
    _state = state;
    response.Method = _request.Method;
    response.Url = _request.Url;
    _response = std::make_shared<HttpMessage>(std::move(response));
  }
  state = _state;
  auto handler = _responseHandler;
  // the response handler should only fire once and might be
  // holding a reference to other objects, so we need to clear it
  _responseHandler = nullptr;
  lock.unlock();
  if (handler && (state == HttpRequestState::Complete ||
                  state == HttpRequestState::Error ||
                  state == HttpRequestState::Cancelled)) {
    handler(_response);
  }
}

bool HttpClientPendingRequest::SendRequest(HttpConnection &conn) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  if (_state == HttpRequestState::Cancelled) {
    return false;
  }
  _state = HttpRequestState::Requesting;
  lock.unlock();
  SendHttpRequest(conn.Connection, conn.Origin->Host, _request);

  if (_request.Error.size()) {
    conn.Request.reset();
    lock.lock();
    _state = HttpRequestState::Error;
    return false;
  } else {
    conn.Request = shared_from_this();
    conn.KeepAlive = mg_millis() + conn.KeepAliveDuration;
    return true;
  }
}

void HttpClientPendingRequest::ReceiveResponse(HttpConnection &conn,
                                               mg_http_message *hm) {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  if (_state == HttpRequestState::Cancelled) {
    return;
  }
  lock.unlock();

  const auto connectionHeader = mg_http_get_header(hm, S_CONNECTION.c_str());
  if (connectionHeader &&
      mg_strcmp(*connectionHeader, mg_stdstr(S_CLOSE)) == 0) {
    conn.KeepAliveDuration = 0;
    conn.KeepAlive = mg_millis();
  }

  if (conn.KeepAliveDuration > 0) {
    const auto keepAliveHeader = mg_http_get_header(hm, S_KEEP_ALIVE.c_str());
    if (keepAliveHeader) {
      int timeout = -1, maxRequests = -1;
      ParseKeepAliveHeader(keepAliveHeader, timeout, maxRequests);
      if (timeout > 0) {
        conn.KeepAliveDuration = timeout;
      } else if (maxRequests == 0) {
        conn.KeepAliveDuration = 0;
      }
    }
  }
  conn.KeepAlive = mg_millis() + conn.KeepAliveDuration;

  HttpMessage response;
  CreateHttpMessage(hm, response);
  SetResponse(response.Error.size() ? HttpRequestState::Error
                                    : HttpRequestState::Complete,
              response);
}

HttpClientRequest::HttpClientRequest(
    const std::string &url,
    std::shared_ptr<MongooseNetworkManagerComponent> manager)
    : _manager(manager) {
  _request.Method = S_GET;
  _request.Url = url;
}

IHttpClientRequest *HttpClientRequest::SetRequestMethod(
    const std::string &method) {
  _request.Method = method;
  return this;
}

IHttpClientRequest *HttpClientRequest::SetRequestHeader(
    const std::string &header, const std::string &value) {
  _request.Headers.insert(std::make_pair(header, value));
  return this;
}

IHttpClientRequest *HttpClientRequest::SetRequestBody(const char *body,
                                                      size_t bodyLength,
                                                      bool compress) {
  _request.Body.assign(body, bodyLength);
  if (compress) {
    _request.Headers.insert(std::make_pair(S_CONTENT_ENCODING, S_GZIP));
  }
  return this;
}

bool HttpClientRequest::GetRequestHeader(const std::string &header,
                                         std::string &value) const {
  const auto found = _request.Headers.find(header);
  if (found != _request.Headers.end()) {
    value = found->second;
    return true;
  }
  return false;
}

std::shared_ptr<IHttpClientPendingRequest> HttpClientRequest::SendRequest(
    std::function<void(std::shared_ptr<HttpMessage> &response)> handler) {
  auto pending = _manager->SendRequest(_request, handler);
  return pending;
}

IWebSocket *WebSocketBase::Send(const std::vector<char> &data, bool binary) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  auto &buffer = _out.emplace_back(data.size(), binary);
  memcpy_s(buffer.Data.data(), buffer.Data.size(), data.data(), data.size());
  return this;
}

IWebSocket *WebSocketBase::Send(void *data, size_t dataLength, bool binary) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  auto &buffer = _out.emplace_back(dataLength, binary);
  memcpy_s(buffer.Data.data(), buffer.Data.size(), data, dataLength);
  return this;
}

IWebSocket *WebSocketBase::OnReceive(
    std::function<void(std::span<const char> &message, bool binary)> handler) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _inHandler = handler;
  return this;
}

MGDFWebSocketConnectionState WebSocketBase::GetConnectionState(
    std::string &lastError) const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  lastError = _lastError;
  return _state;
}

WebSocket::WebSocket(const std::string &url,
                     std::shared_ptr<MongooseNetworkManagerComponent> manager,
                     size_t reconnectInterval)
    : WebSocketBase(MGDF_WEBSOCKET_CLOSED),
      _url(url),
      _manager(manager),
      _reconnectInterval(reconnectInterval),
      _closing(false),
      _forceClose(false) {
  const auto host = mg_url_host(url.c_str());
  _host = std::string(host.ptr, host.len);
  _usesTLS = mg_url_is_ssl(url.c_str());
}

WebSocket::~WebSocket() {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _forceClose = true;
  _closing = true;
  _cv.wait(lock, [this]() { return _state == MGDF_WEBSOCKET_CLOSED; });
}

void WebSocket::Connect(mg_mgr &mgr) {
  {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _state = MGDF_WEBSOCKET_CONNECTING;
  }
  mg_ws_connect(&mgr, _url.c_str(), &WebSocket::HandleEvents, this, nullptr);
}

void WebSocket::HandleEvents(mg_connection *conn, int ev, void *ev_data,
                             void *fn_data) {
  WebSocket *socket = static_cast<WebSocket *>(fn_data);
  switch (ev) {
    case MG_EV_POLL: {
      std::unique_lock<std::shared_mutex> lock(socket->_mutex);
      if (socket->_closing) {
        conn->is_closing = 1;
        return;
      } else if (conn->is_websocket) {
        if (!socket->_out.size()) {
          return;
        }
        // send outgoing requests
        std::vector<WebSocketMessage> out = std::move(socket->_out);
        socket->_out.clear();
        lock.unlock();
        for (const auto &o : out) {
          mg_ws_send(conn, o.Data.data(), o.Data.size(),
                     o.Binary ? WEBSOCKET_OP_BINARY : WEBSOCKET_OP_TEXT);
        }
      }
    } break;
    case MG_EV_CONNECT: {
      if (socket->_usesTLS) {
        socket->_manager->TLSInit(conn, socket->_host);
      }
    } break;
    case MG_EV_HTTP_MSG: {
      mg_ws_upgrade(conn, (struct mg_http_message *)ev_data, NULL);
    } break;
    case MG_EV_WS_OPEN: {
      std::unique_lock<std::shared_mutex> lock(socket->_mutex);
      socket->_state = MGDF_WEBSOCKET_OPEN;
      socket->_lastError.clear();
    } break;
    case MG_EV_WS_MSG: {
      struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
      const auto op = wm->flags & 15;
      auto data = std::span<const char>(wm->data.ptr, wm->data.len);
      std::shared_lock<std::shared_mutex> lock(socket->_mutex);
      auto inHandler = socket->_inHandler;
      lock.unlock();
      if (inHandler) {
        inHandler(data, op == WEBSOCKET_OP_BINARY);
      }
    } break;
    case MG_EV_ERROR: {
      std::unique_lock<std::shared_mutex> lock(socket->_mutex);
      socket->_lastError = std::string((const char *)ev_data);
    } break;
    case MG_EV_CLOSE: {
      std::unique_lock<std::shared_mutex> lock(socket->_mutex);
      socket->_state = MGDF_WEBSOCKET_CLOSED;
      if (!socket->_forceClose && socket->_reconnectInterval) {
        lock.unlock();
        socket->_manager->ReconnectWebSocket(
            socket->weak_from_this(), mg_millis() + socket->_reconnectInterval);
      } else {
        lock.unlock();
        socket->_cv.notify_one();
      }
    } break;
    default:
      break;
  }
}

ServerWebSocket::ServerWebSocket(mg_connection *connection,
                                 std::shared_ptr<HttpServer> server)
    : WebSocketBase(MGDF_WEBSOCKET_OPEN),
      _connection(connection),
      _server(server) {}

ServerWebSocket::~ServerWebSocket() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  if (_state == MGDF_WEBSOCKET_CLOSED) {
    return;
  }
  lock.unlock();
  _server->QueueClose(_connection);
}

void ServerWebSocket::ReceiveMessage(std::span<const char> &message,
                                     bool binary) {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  auto inHandler = _inHandler;
  if (_state == MGDF_WEBSOCKET_CLOSED) {
    return;
  }
  lock.unlock();
  if (inHandler) {
    inHandler(message, binary);
  }
}

IWebSocket *ServerWebSocket::Send(const std::vector<char> &data, bool binary) {
  auto result = WebSocketBase::Send(data, binary);
  _server->QueueMessage(shared_from_this());
  return result;
}

IWebSocket *ServerWebSocket::Send(void *data, size_t dataLength, bool binary) {
  auto result = WebSocketBase::Send(data, dataLength, binary);
  _server->QueueMessage(shared_from_this());
  return result;
}

void ServerWebSocket::SendMessages() {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  std::vector<WebSocketMessage> out = std::move(_out);
  _out.clear();
  if (_state == MGDF_WEBSOCKET_CLOSED) {
    return;
  }
  lock.unlock();
  for (const auto &o : out) {
    mg_ws_send(_connection, o.Data.data(), o.Data.size(),
               o.Binary ? WEBSOCKET_OP_BINARY : WEBSOCKET_OP_TEXT);
  }
}

void ServerWebSocket::ForceClose() {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _state = MGDF_WEBSOCKET_CLOSED;
}

HttpServer::HttpServer(const std::string &webSocketPath,
                       std::shared_ptr<MongooseNetworkManagerComponent> manager)
    : _closing(false),
      _closed(true),
      _listening(false),
      _manager(manager),
      _webSocketPath(webSocketPath) {}

HttpServer::~HttpServer() {
  _closing.store(true);
  std::unique_lock<std::mutex> lock(_mutex);
  _cv.wait(lock, [this]() { return _closed; });
}

IHttpServer *HttpServer::OnHttpRequest(
    std::function<void(std::shared_ptr<IHttpServerRequest> request)> handler) {
  std::unique_lock<std::mutex> lock(_mutex);
  _requestHandler = handler;
  return this;
}

IHttpServer *HttpServer::OnWebSocketRequest(
    std::function<void(std::shared_ptr<IWebSocket> socket)> handler) {
  std::unique_lock<std::mutex> lock(_mutex);
  _webSocketHandler = handler;
  return this;
}

void HttpServer::SendResponse(mg_connection *conn, HttpMessage &response) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_closed) {
    return;
  }
  // make sure the connection is still valid before we add a pending response
  const auto &awaiting = _awaitingServerResponse.find(conn);
  if (awaiting != _awaitingServerResponse.end()) {
    _pendingServerResponses.insert(std::make_pair(conn, std::move(response)));
    _awaitingServerResponse.erase(awaiting);
  }
}

void HttpServer::QueueMessage(std::shared_ptr<ServerWebSocket> socket) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_closed) {
    return;
  }
  _pendingWebSocketMessages.insert(socket);
}

void HttpServer::QueueClose(mg_connection *connection) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_closed) {
    return;
  }
  _webSockets.erase(connection);
  _pendingWebSocketClosures.insert(connection);
}

void HttpServer::Listen(mg_mgr &mgr, uint32_t port) {
  _closed = false;
  std::ostringstream listen;
  listen << "0.0.0.0:" << port;
  mg_http_listen(&mgr, listen.str().c_str(), &HttpServer::HandleEvents, this);
}

void HttpServer::HandleEvents(mg_connection *c, int ev, void *ev_data,
                              void *fn_data) {
  if (!fn_data) {
    return;
  }

  HttpServer *server = static_cast<HttpServer *>(fn_data);
  switch (ev) {
    case MG_EV_OPEN: {
      if (c->is_listening) {
        c->label[0] = 'L';
        server->_listening.store(true);
      }
    } break;
    case MG_EV_POLL: {
      if (server->_closing.load()) {
        c->is_closing = 1;
      }

      if (!c->is_listening) {
        return;
      }

      // send pending server http responses
      std::unique_lock<std::mutex> serverLock(server->_mutex);
      auto responses = std::move(server->_pendingServerResponses);
      server->_pendingServerResponses.clear();
      serverLock.unlock();
      for (auto &response : responses) {
        SendHttpResponse(response.first, response.second);
      }

      std::unique_lock<std::mutex> socketLock(server->_mutex);
      // close any pending sockets
      for (auto connection : server->_pendingWebSocketClosures) {
        connection->is_closing = 1;
      }
      server->_pendingWebSocketClosures.clear();
      auto pendingMessages = std::move(server->_pendingWebSocketMessages);
      server->_pendingWebSocketMessages.clear();
      socketLock.unlock();
      // send pending socket data
      for (auto &socket : pendingMessages) {
        socket->SendMessages();
      }
    } break;
    case MG_EV_HTTP_MSG: {
      mg_http_message *hm = (mg_http_message *)ev_data;
      if (!server->_webSocketPath.empty() &&
          hm->uri.len >= server->_webSocketPath.size() &&
          strncmp(hm->uri.ptr, server->_webSocketPath.c_str(),
                  server->_webSocketPath.size()) == 0) {
        mg_ws_upgrade(c, hm, NULL);
        c->label[0] = 'W';
      } else {
        std::unique_lock<std::mutex> lock(server->_mutex);
        auto handler = server->_requestHandler;
        if (!handler) {
          // if the server has no http handler, kill the client connection
          c->is_closing = 1;
        } else {
          server->_awaitingServerResponse.insert(c);
          lock.unlock();
          HttpMessage message;
          CreateHttpMessage(hm, message);
          c->label[0] = 'H';
          auto response = std::make_shared<HttpServerRequest>(
              c, message, server->shared_from_this());
          handler(response);
        }
      }
    } break;
    case MG_EV_WS_MSG: {
      std::shared_ptr<ServerWebSocket> socket;
      {
        std::unique_lock<std::mutex> lock(server->_mutex);
        auto found = server->_webSockets.find(c);
        if (found == server->_webSockets.end()) {
          auto handler = server->_webSocketHandler;
          if (handler) {
            socket = std::make_shared<ServerWebSocket>(
                c, server->shared_from_this());
            server->_webSockets.insert(std::make_pair(c, socket));
            lock.unlock();
          } else {
            // no socket connection handler, so just disconnect the client
            c->is_closing = 1;
            return;
          }
          handler(std::static_pointer_cast<IWebSocket>(socket));
        } else if (!(socket = found->second.lock())) {
          // this could possibly happen if the server websocket destructor is
          // being called but hasn't yet completed and queued the connection to
          // be cleaned up on the server yet. Since in this case the socket will
          // get cleaned up, and we don't want the message to be processed we'll
          // do nothing in this case
          return;
        }
      }
      _ASSERTE(socket);
      struct mg_ws_message *wm = (struct mg_ws_message *)ev_data;
      const auto op = wm->flags & 15;
      auto data = std::span<const char>(wm->data.ptr, wm->data.len);
      socket->ReceiveMessage(data, op == WEBSOCKET_OP_BINARY);
    } break;
    case MG_EV_ERROR: {
      if (c->label[0] == 'W') {
        std::unique_lock<std::mutex> lock(server->_mutex);
        auto found = server->_webSockets.find(c);
        if (found != server->_webSockets.end()) {
          server->_webSockets.erase(found);
          if (auto socket = found->second.lock()) {
            lock.unlock();
            const std::string_view error(static_cast<char *>(ev_data));
            socket->SetLastError(error);
          }
        }
      }
    } break;
    case MG_EV_CLOSE: {
      if (c->label[0] == 'W') {
        // close a connected socket
        std::unique_lock<std::mutex> lock(server->_mutex);
        auto found = server->_webSockets.find(c);
        if (found != server->_webSockets.end()) {
          auto socket = found->second.lock();
          server->_webSockets.erase(found);
          lock.unlock();
          if (socket) {
            socket->ForceClose();
          }
        }
      } else if (c->label[0] == 'H') {
        std::lock_guard<std::mutex> serverLock(server->_mutex);
        server->_pendingServerResponses.erase(c);
        server->_awaitingServerResponse.erase(c);
      } else if (c->label[0] == 'L') {
        // make sure any active connections on this server when closing don't
        // access the server after its deleted
        for (mg_connection *wc = c->mgr->conns; wc != NULL; wc = wc->next) {
          if (wc->fn_data == server &&
              (c->label[0] == 'W' || c->label[0] == 'H' ||
               c->label[0] == 'L')) {
            wc->fn_data = nullptr;
          }
        }
        server->_listening.store(false);
        std::unique_lock<std::mutex> lock(server->_mutex);
        server->_closed = true;
        // break any circular shared_ptr references that would pin the server
        server->_pendingWebSocketMessages.clear();
        lock.unlock();
        server->_cv.notify_one();
      }
    } break;
    default:
      break;
  }
}

void MongooseNetworkManagerComponent::ReconnectWebSocket(
    std::weak_ptr<WebSocket> socket, size_t when) {
  std::lock_guard<std::mutex> lock(_webSocketMutex);
  _pendingWebSockets.push_back(std::make_pair(socket, when));
}

MongooseNetworkManagerComponent::MongooseNetworkManagerComponent(
    const NetworkManagerOptions &options)
    : _running(true), _options(options), _fs(), _mgr() {
  _pollThread = std::thread([this]() {
    MemFS::InitMGFS(_fs);
    MemFS::Ensure(CertificateManager::S_CA_PEM, &CertificateManager::LoadCerts);

    mg_mgr_init(&_mgr);
    while (_running) {
      Poll();
      mg_mgr_poll(&_mgr, 16);
    }
    mg_mgr_free(&_mgr);
  });
}

void MongooseNetworkManagerComponent::Stop() {
  if (_running) {
    _running = false;
    _pollThread.join();
  }
}

std::shared_ptr<HttpClientPendingRequest>
MongooseNetworkManagerComponent::SendRequest(
    HttpMessage &request,
    std::function<void(std::shared_ptr<HttpMessage> &response)> handler) {
  // transformed url
  std::string url = request.Url;
  std::transform(url.begin(), url.end(), url.begin(), [](auto ch) {
    return std::tolower(ch, std::locale("en_US.utf8"));
  });
  // original url
  const char *originalUrl = mg_url_uri(request.Url.c_str());

  auto pending = std::make_shared<HttpClientPendingRequest>(request, handler);

  if (!url.starts_with(S_HTTP) && !url.starts_with(S_HTTPS)) {
    HttpMessage response = {.Error = "Invalid URL protocol"};
    pending->SetResponse(HttpRequestState::Error, response);
    return pending;
  }
  const struct mg_str host = mg_url_host(url.c_str());
  const unsigned short port = mg_url_port(url.c_str());
  const bool usesTLS = mg_url_is_ssl(url.c_str());

  // to tell origins http/https origins using default ports
  // apart we need to include the port in all cases
  std::ostringstream canonicalHost;
  std::ostringstream canonicalHostAndPort;
  canonicalHost << std::string(host.ptr, host.len);
  canonicalHostAndPort << std::string(host.ptr, host.len) << ":" << port;
  if ((usesTLS && port != 443) || !(usesTLS && port != 80)) {
    canonicalHost << ":" << port;
  }
  std::string canonicalHostStr = canonicalHost.str();
  std::string canonicalHostAndPortStr = canonicalHostAndPort.str();

  std::ostringstream canonicalURL;
  canonicalURL << (usesTLS ? S_HTTPS : S_HTTP) << canonicalHostStr
               << (originalUrl[0] == '\\' ? "/" : originalUrl);

  std::unique_lock<std::shared_mutex> lock(_originMutex);
  auto o = _origins.find(canonicalHostAndPort.str());
  if (o == _origins.end()) {
    o = _origins
            .emplace(
                canonicalHostAndPortStr,
                std::shared_ptr<HttpOrigin>(new HttpOrigin{
                    .TLSInit =
                        [this, usesTLS, canonicalHostStr](auto conn) {
                          if (usesTLS) {
                            TLSInit(conn, canonicalHostStr);
                          }
                        },
                    .ConnectionLimit = _options.HttpClientOriginConnectionLimit,
                    .Host = canonicalHostStr,
                }))
            .first;
  }
  lock.unlock();

  {
    std::lock_guard originLock(o->second->Mutex);
    o->second->PendingRequests.push_back(pending);
  }
  return pending;
}

MongooseNetworkManagerComponent::~MongooseNetworkManagerComponent() {
  Stop();

  // clean up any pending requests or requests in progress
  std::shared_lock<std::shared_mutex> lock(_originMutex);
  for (auto &origin : _origins) {
    std::lock_guard<std::mutex> originLock(origin.second->Mutex);
    for (const auto pending : origin.second->PendingRequests) {
      HttpMessage response = {.Code = -1, .Error = S_CANCELLED};
      pending->SetResponse(HttpRequestState::Cancelled, response);
    }
    for (auto &c : origin.second->Connections) {
      if (c.second->Request) {
        const auto &pending = c.second->Request;
        HttpMessage response = {.Code = -1, .Error = S_CANCELLED};
        pending->SetResponse(HttpRequestState::Cancelled, response);
      }
    }
  }
}

std::unique_ptr<IHttpClientRequest>
MongooseNetworkManagerComponent::CreateHttpRequest(const std::string &url) {
  return std::make_unique<HttpClientRequest>(url, shared_from_this());
}

std::shared_ptr<IHttpServer> MongooseNetworkManagerComponent::CreateHttpServer(
    uint32_t port, const std::string &socketPath) {
  auto server = std::make_shared<HttpServer>(socketPath, shared_from_this());
  std::lock_guard<std::mutex> lock(_serverMutex);
  _pendingServers.push_back(std::make_pair(server, port));
  return server;
}

std::shared_ptr<IWebSocket> MongooseNetworkManagerComponent::CreateWebSocket(
    const std::string &url) {
  auto newWebSocket = std::make_shared<WebSocket>(
      url, shared_from_this(), _options.WebSocketClientReconnectInterval);
  std::lock_guard<std::mutex> lock(_webSocketMutex);
  _pendingWebSockets.push_back(std::make_pair(newWebSocket, mg_millis()));
  return newWebSocket;
}

void MongooseNetworkManagerComponent::Poll() {
  {
    std::shared_lock<std::shared_mutex> lock(_originMutex);
    for (auto &origin : _origins) {
      std::lock_guard<std::mutex> originLock(origin.second->Mutex);
      // any pending requests that have not been assigned to a connection
      // will require a new connection to be created. If we've maxxed out the
      // allowable connections to this domain, then the requests will stay
      // pending until some requests complete
      while (origin.second->PendingRequests.size() &&
             origin.second->Connections.size() <
                 origin.second->ConnectionLimit) {
        auto p = origin.second->PendingRequests.front();
        origin.second->PendingRequests.pop_front();

        auto connection = std::make_shared<HttpConnection>();
        connection->Request = p;
        connection->KeepAliveDuration = _options.HttpClientKeepAlive;
        connection->ConnectTimeoutDuration =
            _options.HttpClientConnectionTimeout;
        connection->Origin = origin.second.get();

        auto connectionId = mg_http_connect(
            &_mgr, p->GetUrl().c_str(),
            &MongooseNetworkManagerComponent::HandleEvents, connection.get());
        origin.second->Connections.insert(
            std::make_pair(connectionId, connection));
      }
    }
  }

  {
    // create any pending websocket client connections
    std::lock_guard<std::mutex> websocketLock(_webSocketMutex);
    const auto now = mg_millis();
    auto it = _pendingWebSockets.begin();
    while (it != _pendingWebSockets.end()) {
      if (now >= it->second) {
        auto socket = it->first.lock();
        if (socket) {
          socket->Connect(_mgr);
        }
        it = _pendingWebSockets.erase(it);
      } else {
        ++it;
      }
    }
  }

  {
    // create any pending http servers to begin listening
    std::lock_guard<std::mutex> serverLock(_serverMutex);
    auto it = _pendingServers.begin();
    while (it != _pendingServers.end()) {
      auto server = it->first.lock();
      if (server) {
        server->Listen(_mgr, it->second);
      }
      it = _pendingServers.erase(it);
    }
    _pendingServers.clear();
  }
}

void MongooseNetworkManagerComponent::TLSInit(mg_connection *conn,
                                              const std::string &host) {
  const mg_tls_opts opts = {.ca = CertificateManager::S_CA_PEM.c_str(),
                            .srvname = mg_stdstr(host),
                            .fs = &_fs};
  mg_tls_init(conn, &opts);
}

void MongooseNetworkManagerComponent::HandleEvents(mg_connection *c, int ev,
                                                   void *ev_data,
                                                   void *fn_data) {
  HttpConnection *conn = static_cast<HttpConnection *>(fn_data);
  _ASSERTE(conn);
  if (ev == MG_EV_OPEN) {
    conn->ConnectTimeout = mg_millis() + conn->ConnectTimeoutDuration;
    conn->KeepAlive = mg_millis() + conn->KeepAliveDuration;
    conn->Connection = c;
  } else if (ev == MG_EV_POLL) {
    if (conn->Request) {
      // if we have an active request associated with this connection
      // check for connection timeout
      if (mg_millis() > conn->ConnectTimeout &&
          (c->is_connecting || c->is_resolving)) {
        // close the connection
        mg_error(c, "Connect timeout");
      } else if (conn->Request->GetRequestState() ==
                 HttpRequestState::Cancelled) {
        // close the connection to ensure the request is not processed
        c->is_closing = true;
      }
    } else {
      // this connection is idle
      // timeout any idle keepalive connections
      if (mg_millis() > conn->KeepAlive) {
        c->is_draining = 1;
      } else {
        auto origin = conn->Origin;
        std::lock_guard<std::mutex> lock(origin->Mutex);
        if (!origin->PendingRequests.empty()) {
          // if we have some pending requests, then we can reuse this
          // connection
          auto pending = origin->PendingRequests.front();
          origin->PendingRequests.pop_front();
          pending->SendRequest(*conn);
        }
      }
    }
  } else if (ev == MG_EV_CONNECT) {
    conn->Origin->TLSInit(c);
    if (!conn->Request->SendRequest(*conn)) {
      if (conn->KeepAliveDuration > 0) {
        // we're not making a request, but we'll try to re-use this connection
        c->is_resp = 0;
      }
    }
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    conn->Request->ReceiveResponse(*conn, hm);
    conn->Request.reset();
    if (conn->KeepAliveDuration > 0) {
      // re-use this connection
      c->is_resp = 0;
    }
  } else if (ev == MG_EV_ERROR) {
    if (conn->Request) {
      HttpMessage response = {
          .Error = std::string(static_cast<char *>(ev_data)),
      };
      conn->Request->SetResponse(HttpRequestState::Error, response);
      conn->Request.reset();
    }
  } else if (ev == MG_EV_CLOSE) {
    auto origin = conn->Origin;
    std::lock_guard<std::mutex> lock(origin->Mutex);
    origin->Connections.erase(c);
  }
}

void ParseKeepAliveHeader(const mg_str *header, int &timeout, int &max) {
  if (header == nullptr || header->len == 0) {
    return;
  }

  const char *start = header->ptr;
  const char *end = start + header->len;

  while (start < end) {
    // Skip leading whitespace
    while (start < end && std::isspace(*start)) ++start;

    // Check if it's "timeout" or "max"
    if (start + 7 <= end &&
        mg_strcmp(mg_str_n(start, 7), mg_stdstr(S_TIMEOUT)) == 0) {
      start += 7;
      if (*start == '=') {
        ++start;
        timeout = 0;
        while (start < end && std::isdigit(*start)) {
          timeout = timeout * 10 + (*start - '0');
          ++start;
        }
      }
    } else if (start + 3 <= end &&
               mg_strcmp(mg_str_n(start, 3), mg_stdstr(S_MAX)) == 0) {
      start += 3;
      if (*start == '=') {
        ++start;
        max = 0;
        while (start < end && std::isdigit(*start)) {
          max = max * 10 + (*start - '0');
          ++start;
        }
      }
    }

    // Move to next directive
    while (start < end && *start != ',') ++start;
    if (start < end && *start == ',') ++start;
  }
}

}  // namespace mongoose
}  // namespace network
}  // namespace core
}  // namespace MGDF