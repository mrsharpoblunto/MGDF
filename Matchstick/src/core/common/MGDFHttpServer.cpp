#include "stdafx.h"

#include "MGDFHttpServer.hpp"

#include "MGDFHttpCommon.hpp"
#include "MGDFLoggerImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

static const std::string S_EMPTY("");
static const std::string S_IDENTITY("");

namespace MGDF {
namespace core {

WebSocketServerConnection::WebSocketServerConnection(mg_connection *c,
                                                     HttpServer *server)
    : WebSocketConnectionBase(c, MGDF_WEBSOCKET_OPEN),
      _server(server),
      _serverMutex(server->_mutex) {}

WebSocketServerConnection::~WebSocketServerConnection() {
  std::lock_guard<std::mutex> lock(*_serverMutex);
  if (_server) {
    auto found = _server->_sockets.find(_conn);
    if (found != _server->_sockets.end()) {
      _server->_sockets.emplace(_conn,
                                std::weak_ptr<WebSocketServerConnection>());
    }
  }
}

HttpServerRequest::HttpServerRequest(mg_connection *c,
                                     struct mg_http_message *m,
                                     HttpServer *server)
    : _conn(c),
      _request(CreateHttpMessage<HttpMessageBase>(m)),
      _server(server),
      _url(m->uri.ptr, m->uri.len),
      _serverMutex(server->_mutex) {
  std::string encoding(S_IDENTITY);
  std::string acceptEncoding;
  if (GetRequestHeader(S_ACCEPT_ENCODING, acceptEncoding) &&
      acceptEncoding.find(S_GZIP) != std::string_view::npos) {
    encoding = S_GZIP;
  }
  _response.Headers.insert(std::make_pair(S_CONTENT_ENCODING, encoding));
}

bool HttpServerRequest::GetRequestHeader(const std::string &name,
                                         std::string &value) const {
  auto header = _request->Headers.find(name);
  if (header != _request->Headers.end()) {
    value = header->second;
    return true;
  }
  return false;
}

bool HttpServerRequest::HasRequestHeader(const std::string &name) const {
  auto header = _request->Headers.find(name);
  return header != _request->Headers.end();
}

const std::string &HttpServerRequest::GetRequestHeader(
    const std::string &name) const {
  return _request->Headers.at(name);
}

const std::string &HttpServerRequest::GetRequestMethod() const {
  return _request->Method;
}

const std::string &HttpServerRequest::GetRequestUrl() const { return _url; }

const std::string &HttpServerRequest::GetRequestBody() const {
  return _request->Body;
}

HttpServerRequest *HttpServerRequest::SetResponseCode(int code) {
  _response.Code = code;
  return this;
}
HttpServerRequest *HttpServerRequest::SetResponseHeader(
    const std::string &name, const std::string &value) {
  _response.Headers.insert(std::make_pair(name, value));
  return this;
}
HttpServerRequest *HttpServerRequest::SetResponseMethod(
    const std::string &method) {
  _response.Method = method;
  return this;
}
HttpServerRequest *HttpServerRequest::SetResponseBody(const char *body,
                                                      UINT64 bodyLength) {
  _response.Body.assign(body, bodyLength);
  auto acceptEncoding = _request->Headers.find(S_ACCEPT_ENCODING);
  if (acceptEncoding != _request->Headers.end() &&
      acceptEncoding->second.find(S_GZIP) != std::string::npos) {
    _response.Headers.insert(std::make_pair(S_CONTENT_ENCODING, S_GZIP));
  }
  return this;
}

void HttpServerRequest::SendResponse() {
  _conn = nullptr;
  _request.reset();
  std::lock_guard<std::mutex> lock(*_serverMutex);
  if (_server) {
    _server->_pendingResponses.erase(this);
    _server->_responses.emplace_back(_conn, std::move(_response));
    _server = nullptr;
  }
}

HttpServerRequest::~HttpServerRequest() {
  std::lock_guard<std::mutex> lock(*_serverMutex);
  if (_server) {
    _response.Code = 500;
    _response.Headers.clear();
    _response.Body.clear();
    _server->_pendingResponses.erase(this);
    _server->_responses.emplace_back(_conn, std::move(_response));
  }
}

HttpServer::HttpServer(std::shared_ptr<NetworkEventLoop> &eventLoop)
    : _eventLoop(eventLoop),
      _conn(nullptr),
      _mutex(std::make_shared<std::mutex>()) {
  _eventLoop->Add(this);
}

HttpServer::~HttpServer() {
  _eventLoop->Remove(this);

  std::unique_lock<std::mutex> lock(*_mutex);
  auto pendingResponses = std::move(_pendingResponses);
  auto sockets = std::move(_sockets);
  lock.unlock();
  for (auto p : pendingResponses) {
    auto r = p.second.lock();
    if (r) {
      r->_server = nullptr;
    }
  }
  for (auto socket : sockets) {
    auto s = socket.second.lock();
    if (s) {
      s->_server = nullptr;
    }
  }
}

void HttpServer::Listen(const std::string &port,
                        const std::string &socketPath) {
  std::lock_guard<std::mutex> lock(*_mutex);
  if (_conn) {
    return;
  }
  _pendingListen = "0.0.0.0:" + port;
  _socketPath = socketPath;
}

void HttpServer::OnPoll(mg_mgr &mgr, mg_fs &) {
  std::lock_guard<std::mutex> lock(*_mutex);
  if (!_conn && !_pendingListen.empty()) {
    _conn = mg_http_listen(&mgr, _pendingListen.c_str(),
                           &HttpServer::HandleRequest, this);
    _pendingListen.clear();
  }
}

void HttpServer::HandleRequest(mg_connection *c, int ev, void *ev_data,
                               void *fn_data) {
  std::ignore = c;
  std::ignore = ev_data;
  std::ignore = ev;
  HttpServer *server = static_cast<HttpServer *>(fn_data);
  switch (ev) {
    case MG_EV_POLL: {
      std::unique_lock<std::mutex> lock(*server->_mutex);

      // if there are http responses to send, send them
      if (server->_responses.size()) {
        auto responses = std::move(server->_responses);
        server->_responses.clear();
        lock.unlock();
        for (auto &response : responses) {
          SendHttpResponse(response.first, response.second);
        }
        lock.lock();
      }

      // if this is a socket, poll for updates or clean it up
      auto socket = server->_sockets.find(c);
      if (socket != server->_sockets.end()) {
        lock.unlock();
        auto s = socket->second.lock();
        if (s) {
          s->Poll(ev, ev_data, fn_data);
        } else {
          c->is_closing = 1;
          lock.lock();
          server->_sockets.erase(c);
          lock.unlock();
        }
      }
    } break;
    case MG_EV_HTTP_MSG: {
      struct mg_http_message *hm = (struct mg_http_message *)ev_data;
      if (!server->_socketPath.empty() &&
          mg_match(
              hm->uri,
              mg_str_n(server->_socketPath.c_str(), server->_socketPath.size()),
              NULL)) {
        mg_ws_upgrade(c, hm, NULL);
      } else {
        std::unique_lock<std::mutex> lock(*server->_mutex);
        auto response = std::make_shared<HttpServerRequest>(c, hm, server);
        server->_pendingResponses.insert(
            std::make_pair<HttpServerRequest *,
                           std::weak_ptr<HttpServerRequest>>(
                response.get(), std::weak_ptr<HttpServerRequest>(response)));
        lock.unlock();
        server->OnRequest(response);
      }
    } break;
    case MG_EV_WS_MSG: {
      std::unique_lock<std::mutex> lock(*server->_mutex);
      auto socket = std::make_shared<WebSocketServerConnection>(c, server);
      server->_sockets.insert(
          std::make_pair(c, std::weak_ptr<WebSocketServerConnection>(socket)));
      lock.unlock();
      server->OnSocketRequest(socket);
    } break;
    case MG_EV_ERROR: {
      LOG(static_cast<char *>(ev_data), MGDF_LOG_ERROR);
    } break;
  }
}

}  // namespace core
}  // namespace MGDF