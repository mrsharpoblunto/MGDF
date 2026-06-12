#include "stdafx.h"

#include "HttpServer.hpp"

#include <sstream>
#include <tuple>

#include "../../common/MGDFLoggerImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

static const std::string S_EMPTY("");
static const std::string S_GZIP("gzip");
static const std::string S_IDENTITY("identity");
static const std::string S_CONTENT_ENCODING("Content-Encoding");
static const std::string S_ACCEPT_ENCODING("Accept-Encoding");
static const std::string S_CONTENT_LENGTH("Content-Length");

namespace MGDF {
namespace core {
namespace network {
namespace impl {

HttpServerRequest::HttpServerRequest(
    HttpMessage &request, std::shared_ptr<PendingHttpResponse> pending)
    : _sent(false), _request(std::move(request)), _pending(pending) {
  _response.Method = _request.Method;
  _response.Url = _request.Url;
  std::string encoding(S_IDENTITY);
  std::string acceptEncoding;
  if (GetRequestHeader(S_ACCEPT_ENCODING, acceptEncoding) &&
      acceptEncoding.find(S_GZIP) != std::string::npos) {
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
  const auto found = _request.Headers.find(name);
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
  const auto found = _request.Headers.find(name);
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
    _sent = true;
    std::lock_guard<std::mutex> lock(_pending->Mutex);
    if (!_pending->Ready) {
      _pending->Response = std::move(_response);
      _pending->Ready = true;
      _pending->Cv.notify_all();
    }
  }
}

ServerWebSocket::ServerWebSocket(mg_connection *connection,
                                 std::shared_ptr<HttpServer> server)
    : WebSocketBase(MGDF_WEBSOCKET_OPEN),
      _connection(connection),
      _server(server),
      _fragmentBinary(false) {}

ServerWebSocket::~ServerWebSocket() {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  if (_state == MGDF_WEBSOCKET_CLOSED) {
    return;
  }
  lock.unlock();
  _server->CloseWebSocket(_connection);
}

IWebSocket *ServerWebSocket::Send(const std::vector<char> &data, bool binary) {
  SendFrame(data.data(), data.size(), binary);
  return this;
}

IWebSocket *ServerWebSocket::Send(void *data, size_t dataLength, bool binary) {
  SendFrame(data, dataLength, binary);
  return this;
}

void ServerWebSocket::SendFrame(const void *data, size_t dataLength,
                                bool binary) {
  // holding a shared lock prevents the connection close handler from
  // completing (and civetweb from then freeing the connection) mid-write
  std::shared_lock<std::shared_mutex> lock(_mutex);
  if (_state != MGDF_WEBSOCKET_OPEN) {
    return;
  }
  mg_lock_connection(_connection);
  mg_websocket_write(
      _connection,
      binary ? MG_WEBSOCKET_OPCODE_BINARY : MG_WEBSOCKET_OPCODE_TEXT,
      static_cast<const char *>(data), dataLength);
  mg_unlock_connection(_connection);
}

void ServerWebSocket::ReceiveFrame(int bits, const char *data,
                                   size_t dataLength) {
  const int opcode = bits & 0xf;
  const bool fin = (bits & 0x80) != 0;
  const bool binary = opcode == MG_WEBSOCKET_OPCODE_BINARY;

  if (opcode == MG_WEBSOCKET_OPCODE_CONTINUATION || !fin ||
      !_fragments.empty()) {
    if (opcode != MG_WEBSOCKET_OPCODE_CONTINUATION) {
      _fragmentBinary = binary;
    }
    _fragments.insert(_fragments.end(), data, data + dataLength);
    if (!fin) {
      return;
    }
    auto message = std::span<const char>(_fragments.data(), _fragments.size());
    Deliver(message, _fragmentBinary);
    _fragments.clear();
  } else {
    auto message = std::span<const char>(data, dataLength);
    Deliver(message, binary);
  }
}

void ServerWebSocket::Deliver(std::span<const char> &message, bool binary) {
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

void ServerWebSocket::ForceClose() {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _state = MGDF_WEBSOCKET_CLOSED;
}

HttpServer::HttpServer(const std::string &webSocketPath)
    : _ctx(nullptr),
      _listening(false),
      _closing(false),
      _webSocketPath(webSocketPath) {}

HttpServer::~HttpServer() {
  _closing.store(true);
  {
    // unblock any worker threads still waiting on a consumer response
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto &pending : _pendingResponses) {
      std::lock_guard<std::mutex> pendingLock(pending->Mutex);
      if (!pending->Ready) {
        pending->Response.Code = 503;
        pending->Response.Body = "Service Unavailable";
        pending->Ready = true;
        pending->Cv.notify_all();
      }
    }
  }
  if (_ctx) {
    _listening.store(false);
    // closes all connections (invoking the websocket close handlers)
    // and joins all worker threads
    mg_stop(_ctx);
  }
}

IHttpServer *HttpServer::OnHttpRequest(
    std::function<void(std::shared_ptr<IHttpServerRequest> request)> handler) {
  std::lock_guard<std::mutex> lock(_mutex);
  _requestHandler = handler;
  return this;
}

IHttpServer *HttpServer::OnWebSocketRequest(
    std::function<void(std::shared_ptr<IWebSocket> socket)> handler) {
  std::lock_guard<std::mutex> lock(_mutex);
  _webSocketHandler = handler;
  return this;
}

void HttpServer::Listen(uint32_t port) {
  const std::string listeningPorts(std::to_string(port));
  const char *options[] = {"listening_ports", listeningPorts.c_str(),
                           "num_threads",     "50",
                           "enable_keep_alive", "yes",
                           "keep_alive_timeout_ms", "10000",
                           nullptr};

  mg_callbacks callbacks = {};
  callbacks.log_message = &HttpServer::LogMessage;

  _ctx = mg_start(&callbacks, nullptr, options);
  if (!_ctx) {
    LOG("Unable to start HTTP server on port " << port, MGDF_LOG_ERROR);
    return;
  }

  mg_set_request_handler(_ctx, "**", &HttpServer::HandleRequest, this);
  if (!_webSocketPath.empty()) {
    mg_set_websocket_handler(_ctx, _webSocketPath.c_str(),
                             &HttpServer::HandleWebSocketConnect,
                             &HttpServer::HandleWebSocketReady,
                             &HttpServer::HandleWebSocketData,
                             &HttpServer::HandleWebSocketClose, this);
  }
  _listening.store(true);
}

void HttpServer::CloseWebSocket(mg_connection *connection) {
  std::lock_guard<std::mutex> lock(_mutex);
  // if the entry has already been removed then the connections close
  // handler has run (or is running) and the connection may no longer be
  // written to safely
  if (_webSockets.erase(connection)) {
    mg_lock_connection(connection);
    mg_websocket_write(connection, MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE,
                       nullptr, 0);
    mg_unlock_connection(connection);
  }
}

int HttpServer::LogMessage(const mg_connection *conn, const char *message) {
  std::ignore = conn;
  Logger::Instance().Log("Civetweb", message, MGDF_LOG_HIGH);
  return 1;
}

int HttpServer::HandleRequest(mg_connection *conn, void *cbdata) {
  HttpServer *server = static_cast<HttpServer *>(cbdata);
  const mg_request_info *info = mg_get_request_info(conn);

  HttpMessage request;
  request.Method = info->request_method ? info->request_method : "";
  request.Url = info->local_uri ? info->local_uri : "";
  if (info->query_string && info->query_string[0]) {
    request.Url += "?";
    request.Url += info->query_string;
  }
  for (int i = 0; i < info->num_headers; ++i) {
    request.Headers.insert(std::make_pair(info->http_headers[i].name,
                                          info->http_headers[i].value));
  }

  std::vector<char> body;
  char buffer[4096] = {};
  for (;;) {
    const int read = mg_read(conn, buffer, sizeof(buffer));
    if (read <= 0) {
      break;
    }
    body.insert(body.end(), buffer, buffer + read);
  }
  DecodeBody(body.data(), body.size(), request);

  std::function<void(std::shared_ptr<IHttpServerRequest>)> handler;
  auto pending = std::make_shared<PendingHttpResponse>();
  {
    std::lock_guard<std::mutex> lock(server->_mutex);
    handler = server->_requestHandler;
    if (!handler || server->_closing.load()) {
      mg_send_http_error(conn, 503, "%s", "Service Unavailable");
      return 503;
    }
    server->_pendingResponses.insert(pending);
  }

  {
    // once the handler has been called the consumer is responsible for
    // responding - if it discards the request without doing so, the
    // requests destructor responds with a 500
    auto serverRequest = std::make_shared<HttpServerRequest>(request, pending);
    handler(serverRequest);
  }

  HttpMessage response;
  {
    std::unique_lock<std::mutex> lock(pending->Mutex);
    pending->Cv.wait(lock, [&pending]() { return pending->Ready; });
    response = std::move(pending->Response);
  }
  {
    std::lock_guard<std::mutex> lock(server->_mutex);
    server->_pendingResponses.erase(pending);
  }

  SendHttpResponse(conn, response);
  // a positive return code tells civetweb the response has already been
  // sent so it must not generate one of its own
  return response.Code > 0 ? response.Code : 500;
}

void HttpServer::SendHttpResponse(mg_connection *conn, const HttpMessage &m) {
  std::vector<char> storage;
  const char *bodyData = nullptr;
  size_t bodyLength = 0;
  bool stripContentEncoding = false;
  EncodeBody(m, storage, bodyData, bodyLength, stripContentEncoding);

  std::ostringstream headers;
  for (const auto &h : m.Headers) {
    if (h.first == S_CONTENT_LENGTH ||
        (h.first == S_CONTENT_ENCODING && stripContentEncoding)) {
      continue;
    }
    headers << h.first << ": " << h.second << "\r\n";
  }

  mg_printf(conn,
            "HTTP/1.1 %d %s\r\n"
            "%s"
            "Content-Length: %u\r\n"
            "\r\n",
            m.Code, GetHttpStatusString(m.Code), headers.str().c_str(),
            static_cast<unsigned int>(bodyLength));
  if (bodyLength) {
    mg_write(conn, bodyData, bodyLength);
  }
}

int HttpServer::HandleWebSocketConnect(const mg_connection *conn,
                                       void *cbdata) {
  std::ignore = conn;
  HttpServer *server = static_cast<HttpServer *>(cbdata);
  std::lock_guard<std::mutex> lock(server->_mutex);
  // if the server has no websocket handler, reject the client connection
  if (!server->_webSocketHandler || server->_closing.load()) {
    return 1;
  }
  return 0;
}

void HttpServer::HandleWebSocketReady(mg_connection *conn, void *cbdata) {
  HttpServer *server = static_cast<HttpServer *>(cbdata);
  auto self = server->weak_from_this().lock();
  if (!self) {
    return;
  }

  std::function<void(std::shared_ptr<IWebSocket>)> handler;
  auto socket = std::make_shared<ServerWebSocket>(conn, self);
  {
    std::lock_guard<std::mutex> lock(server->_mutex);
    handler = server->_webSocketHandler;
    if (!handler) {
      return;
    }
    server->_webSockets.insert(std::make_pair(conn, socket));
  }
  handler(std::static_pointer_cast<IWebSocket>(socket));
}

int HttpServer::HandleWebSocketData(mg_connection *conn, int bits, char *data,
                                    size_t dataLength, void *cbdata) {
  HttpServer *server = static_cast<HttpServer *>(cbdata);
  const int opcode = bits & 0xf;
  switch (opcode) {
    case MG_WEBSOCKET_OPCODE_CONNECTION_CLOSE:
      return 0;
    case MG_WEBSOCKET_OPCODE_PING:
      mg_lock_connection(conn);
      mg_websocket_write(conn, MG_WEBSOCKET_OPCODE_PONG, data, dataLength);
      mg_unlock_connection(conn);
      return 1;
    case MG_WEBSOCKET_OPCODE_PONG:
      return 1;
    default: {
      std::shared_ptr<ServerWebSocket> socket;
      {
        std::lock_guard<std::mutex> lock(server->_mutex);
        const auto found = server->_webSockets.find(conn);
        if (found != server->_webSockets.end()) {
          socket = found->second.lock();
        }
      }
      if (socket) {
        socket->ReceiveFrame(bits, data, dataLength);
      }
      // if the socket is being discarded by the consumer, any messages
      // received in the meantime are dropped
      return 1;
    }
  }
}

void HttpServer::HandleWebSocketClose(const mg_connection *conn,
                                      void *cbdata) {
  HttpServer *server = static_cast<HttpServer *>(cbdata);
  std::shared_ptr<ServerWebSocket> socket;
  {
    std::lock_guard<std::mutex> lock(server->_mutex);
    const auto found = server->_webSockets.find(conn);
    if (found != server->_webSockets.end()) {
      socket = found->second.lock();
      server->_webSockets.erase(found);
    }
  }
  if (socket) {
    socket->ForceClose();
  }
}

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
