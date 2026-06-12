#include "stdafx.h"

#include "NetworkManagerComponentImpl.hpp"

#include <chrono>

#include "../../common/MGDFLoggerImpl.hpp"
#include "HttpCommon.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

static const std::string S_HTTP("http://");
static const std::string S_HTTPS("https://");
static const std::string S_GET("GET");
static const std::string S_HEAD("HEAD");
static const std::string S_GZIP("gzip");
static const std::string S_CANCELLED("Cancelled");
static const std::string S_HOST("Host");
static const std::string S_CONTENT_ENCODING("Content-Encoding");
static const std::string S_ACCEPT_ENCODING("Accept-Encoding");
static const std::string S_CONTENT_LENGTH("Content-Length");

namespace MGDF {
namespace core {
namespace network {
namespace impl {

size_t NetworkManagerComponent::NowMs() {
  return static_cast<size_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

HttpClientPendingRequest::~HttpClientPendingRequest() {
  // the poll thread releases any active connection before
  // dropping its reference
  _ASSERTE(!_curl && !_headers);
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

bool HttpClientPendingRequest::CreateConnection(
    CURLM *multi, const NetworkManagerOptions &options) {
  _curl = curl_easy_init();
  if (!_curl) {
    HttpMessage response = {.Error = "Unable to create connection"};
    SetResponse(HttpRequestState::Error, response);
    return false;
  }

  curl_easy_setopt(_curl, CURLOPT_URL, _request.Url.c_str());
  curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, _request.Method.c_str());
  if (_stricmp(_request.Method.c_str(), S_HEAD.c_str()) == 0) {
    curl_easy_setopt(_curl, CURLOPT_NOBODY, 1L);
  }

  if (!_request.Body.empty()) {
    const char *bodyData = nullptr;
    size_t bodyLength = 0;
    bool stripContentEncoding = false;
    EncodeBody(_request, _requestBody, bodyData, bodyLength,
               stripContentEncoding);
    if (stripContentEncoding) {
      _request.Headers.erase(S_CONTENT_ENCODING);
    }
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDS, bodyData);
    curl_easy_setopt(_curl, CURLOPT_POSTFIELDSIZE_LARGE,
                     static_cast<curl_off_t>(bodyLength));
  }

  for (const auto &h : _request.Headers) {
    if (h.first == S_ACCEPT_ENCODING || h.first == S_CONTENT_LENGTH ||
        h.first == S_HOST) {
      continue;
    }
    std::string line(h.first);
    line.append(": ").append(h.second);
    _headers = curl_slist_append(_headers, line.c_str());
  }
  // requests were previously sent without expect-continue semantics, so
  // suppress curls default Expect: 100-continue header for large bodies
  _headers = curl_slist_append(_headers, "Expect:");
  curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);

  curl_easy_setopt(_curl, CURLOPT_ACCEPT_ENCODING, S_GZIP.c_str());
  curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT_MS,
                   static_cast<long>(options.HttpClientConnectionTimeout));
  curl_easy_setopt(_curl, CURLOPT_MAXAGE_CONN,
                   static_cast<long>(options.HttpClientKeepAlive / 1000));
  curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(_curl, CURLOPT_PROXY, "");
  curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION,
                   &HttpClientPendingRequest::WriteResponseBody);
  curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this);
  curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION,
                   &HttpClientPendingRequest::WriteResponseHeader);
  curl_easy_setopt(_curl, CURLOPT_HEADERDATA, this);
  curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _error);
  // a no-op with the default schannel backend, but if curl is ever rebuilt
  // against openssl this validates against the windows certificate store
  curl_easy_setopt(_curl, CURLOPT_SSL_OPTIONS,
                   static_cast<long>(CURLSSLOPT_NATIVE_CA));

  curl_multi_add_handle(multi, _curl);
  return true;
}

void HttpClientPendingRequest::CompleteConnection(CURLcode result) {
  if (result == CURLE_OK) {
    long code = 0;
    curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &code);
    _working.Code = static_cast<int>(code);
    SetResponse(HttpRequestState::Complete, _working);
  } else {
    HttpMessage response = {.Error = _error[0]
                                         ? std::string(_error)
                                         : std::string(
                                               curl_easy_strerror(result))};
    SetResponse(HttpRequestState::Error, response);
  }
}

void HttpClientPendingRequest::ReleaseConnection(CURLM *multi) {
  if (_curl) {
    curl_multi_remove_handle(multi, _curl);
    curl_easy_cleanup(_curl);
    _curl = nullptr;
  }
  if (_headers) {
    curl_slist_free_all(_headers);
    _headers = nullptr;
  }
}

size_t HttpClientPendingRequest::WriteResponseBody(char *data, size_t size,
                                                   size_t members,
                                                   void *context) {
  HttpClientPendingRequest *self =
      static_cast<HttpClientPendingRequest *>(context);
  const size_t total = size * members;
  self->_working.Body.append(data, total);
  return total;
}

size_t HttpClientPendingRequest::WriteResponseHeader(char *data, size_t size,
                                                     size_t members,
                                                     void *context) {
  HttpClientPendingRequest *self =
      static_cast<HttpClientPendingRequest *>(context);
  const size_t total = size * members;
  std::string_view line(data, total);
  while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
    line.remove_suffix(1);
  }
  if (line.empty()) {
    return total;
  }
  if (line.starts_with("HTTP/")) {
    // a new status line (e.g. following a 1xx informational response)
    // resets any headers collected so far
    self->_working.Headers.clear();
    return total;
  }
  const auto colon = line.find(':');
  if (colon != std::string_view::npos) {
    const std::string_view name = line.substr(0, colon);
    std::string_view value = line.substr(colon + 1);
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t')) {
      value.remove_prefix(1);
    }
    self->_working.Headers.insert(
        std::make_pair(std::string(name), std::string(value)));
  }
  return total;
}

HttpClientRequest::HttpClientRequest(
    const std::string &url, std::shared_ptr<NetworkManagerComponent> manager)
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

WebSocketClient::WebSocketClient(
    const std::string &url, std::shared_ptr<NetworkManagerComponent> manager,
    size_t reconnectInterval)
    : WebSocketBase(MGDF_WEBSOCKET_CLOSED),
      _url(url),
      _manager(manager),
      _reconnectInterval(reconnectInterval),
      _forceClose(false),
      _closing(false),
      _curl(nullptr),
      _connecting(false),
      _sendOffset(0),
      _recvBinary(false) {
  _error[0] = '\0';
}

WebSocketClient::~WebSocketClient() {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _forceClose = true;
  _closing = true;
  _cv.wait(lock, [this]() { return _state == MGDF_WEBSOCKET_CLOSED; });
}

IWebSocket *WebSocketClient::Send(const std::vector<char> &data, bool binary) {
  WebSocketBase::Send(data, binary);
  _manager->Wake();
  return this;
}

IWebSocket *WebSocketClient::Send(void *data, size_t dataLength, bool binary) {
  WebSocketBase::Send(data, dataLength, binary);
  _manager->Wake();
  return this;
}

bool WebSocketClient::Connect(CURLM *multi) {
  {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _state = MGDF_WEBSOCKET_CONNECTING;
  }
  _error[0] = '\0';
  _curl = curl_easy_init();
  if (!_curl) {
    Fail(multi, "Unable to create connection");
    return false;
  }
  curl_easy_setopt(_curl, CURLOPT_URL, _url.c_str());
  // complete the websocket handshake via the multi interface, after which
  // frames are sent/received with curl_ws_send/curl_ws_recv when polling
  curl_easy_setopt(_curl, CURLOPT_CONNECT_ONLY, 2L);
  curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(_curl, CURLOPT_PROXY, "");
  curl_easy_setopt(_curl, CURLOPT_ERRORBUFFER, _error);
  // a no-op with the default schannel backend, but if curl is ever rebuilt
  // against openssl this validates against the windows certificate store
  curl_easy_setopt(_curl, CURLOPT_SSL_OPTIONS,
                   static_cast<long>(CURLSSLOPT_NATIVE_CA));
  curl_multi_add_handle(multi, _curl);
  _connecting = true;
  return true;
}

void WebSocketClient::CompleteConnection(CURLM *multi, CURLcode result) {
  _connecting = false;
  if (result == CURLE_OK) {
    // the easy handle must stay attached to the multi for the lifetime of
    // the websocket - removing it detaches the underlying connection,
    // which breaks any subsequent curl_ws_send/curl_ws_recv calls
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _state = MGDF_WEBSOCKET_OPEN;
    _lastError.clear();
  } else {
    // record the failure - the next Pump invocation transitions the
    // socket to closed and schedules any reconnection
    curl_multi_remove_handle(multi, _curl);
    curl_easy_cleanup(_curl);
    _curl = nullptr;
    if (!_error[0]) {
      strncpy_s(_error, curl_easy_strerror(result), sizeof(_error) - 1);
    }
  }
}

void WebSocketClient::Fail(CURLM *multi, const std::string &error) {
  LOG("WebSocket connection to " << _url << " failed: " << error,
      MGDF_LOG_MEDIUM);
  if (_curl) {
    curl_multi_remove_handle(multi, _curl);
    curl_easy_cleanup(_curl);
    _curl = nullptr;
  }
  // drop any partially transmitted message rather than risk
  // duplicating it on reconnect
  if (_sendOffset > 0 && !_sendQueue.empty()) {
    _sendQueue.pop_front();
  }
  _sendOffset = 0;
  _recvBuffer.clear();

  std::unique_lock<std::shared_mutex> lock(_mutex);
  _lastError = error;
  const bool reconnect = !_forceClose && _reconnectInterval > 0;
  lock.unlock();

  if (reconnect) {
    _manager->ScheduleWebSocketConnect(
        weak_from_this(), NetworkManagerComponent::NowMs() + _reconnectInterval);
  }

  // setting the closed state must be the last access of this object from
  // the poll thread, as it releases any destructor waiting on the socket
  lock.lock();
  _state = MGDF_WEBSOCKET_CLOSED;
  lock.unlock();
  _cv.notify_all();
}

void WebSocketClient::Shutdown(CURLM *multi) {
  if (_curl) {
    curl_multi_remove_handle(multi, _curl);
    curl_easy_cleanup(_curl);
    _curl = nullptr;
  }
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _state = MGDF_WEBSOCKET_CLOSED;
  lock.unlock();
  _cv.notify_all();
}

WebSocketClient::PumpResult WebSocketClient::Pump(CURLM *multi) {
  {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    if (_closing) {
      if (_curl) {
        curl_multi_remove_handle(multi, _curl);
        curl_easy_cleanup(_curl);
        _curl = nullptr;
      }
      _state = MGDF_WEBSOCKET_CLOSED;
      lock.unlock();
      _cv.notify_all();
      return PumpResult::Closed;
    }
  }
  if (_connecting) {
    return PumpResult::Active;
  }
  if (!_curl) {
    // the connection attempt failed
    Fail(multi, _error[0] ? _error : "Connection failed");
    return PumpResult::Closed;
  }

  // send any queued outgoing messages
  {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    for (auto &m : _out) {
      _sendQueue.push_back(std::move(m));
    }
    _out.clear();
  }
  while (!_sendQueue.empty()) {
    const auto &m = _sendQueue.front();
    size_t sent = 0;
    const CURLcode result = curl_ws_send(
        _curl, m.Data.data() + _sendOffset, m.Data.size() - _sendOffset, &sent,
        0, m.Binary ? CURLWS_BINARY : CURLWS_TEXT);
    if (result == CURLE_OK || result == CURLE_AGAIN) {
      _sendOffset += sent;
      if (_sendOffset >= m.Data.size()) {
        _sendQueue.pop_front();
        _sendOffset = 0;
      }
      if (result == CURLE_AGAIN) {
        break;
      }
    } else {
      Fail(multi, curl_easy_strerror(result));
      return PumpResult::Closed;
    }
  }

  // receive any incoming messages
  char buffer[4096] = {};
  while (true) {
    size_t read = 0;
    const curl_ws_frame *meta = nullptr;
    const CURLcode result =
        curl_ws_recv(_curl, buffer, sizeof(buffer), &read, &meta);
    if (result == CURLE_AGAIN) {
      break;
    } else if (result != CURLE_OK || !meta) {
      Fail(multi, result != CURLE_OK ? curl_easy_strerror(result)
                              : "Connection closed");
      return PumpResult::Closed;
    }
    if (meta->flags & CURLWS_CLOSE) {
      Fail(multi, "Connection closed by server");
      return PumpResult::Closed;
    } else if (meta->flags & CURLWS_PING) {
      size_t sent = 0;
      curl_ws_send(_curl, buffer, read, &sent, 0, CURLWS_PONG);
      continue;
    } else if (meta->flags & CURLWS_PONG) {
      continue;
    }

    if (_recvBuffer.empty()) {
      _recvBinary = (meta->flags & CURLWS_BINARY) != 0;
    }
    _recvBuffer.insert(_recvBuffer.end(), buffer, buffer + read);
    if (meta->bytesleft == 0 && !(meta->flags & CURLWS_CONT)) {
      auto message =
          std::span<const char>(_recvBuffer.data(), _recvBuffer.size());
      std::shared_lock<std::shared_mutex> lock(_mutex);
      auto inHandler = _inHandler;
      lock.unlock();
      if (inHandler) {
        inHandler(message, _recvBinary);
      }
      _recvBuffer.clear();
    }
  }
  return PumpResult::Active;
}

NetworkManagerComponent::NetworkManagerComponent(
    const NetworkManagerOptions &options)
    : _options(options), _running(true), _multi(nullptr) {
  _multi = curl_multi_init();
  curl_multi_setopt(_multi, CURLMOPT_MAX_HOST_CONNECTIONS,
                    static_cast<long>(options.HttpClientOriginConnectionLimit));
  _pollThread = std::thread([this]() { Poll(); });
}

NetworkManagerComponent::~NetworkManagerComponent() {
  Stop();
  // clean up anything that was queued after the poll thread exited
  std::lock_guard<std::mutex> lock(_requestMutex);
  for (auto &pending : _newRequests) {
    HttpMessage response = {.Code = -1, .Error = S_CANCELLED};
    pending->SetResponse(HttpRequestState::Cancelled, response);
  }
  _newRequests.clear();
  curl_multi_cleanup(_multi);
}

void NetworkManagerComponent::Stop() {
  if (_running.load()) {
    _running.store(false);
    Wake();
    _pollThread.join();
  }
}

void NetworkManagerComponent::Wake() { curl_multi_wakeup(_multi); }

std::unique_ptr<IHttpClientRequest> NetworkManagerComponent::CreateHttpRequest(
    const std::string &url) {
  return std::make_unique<HttpClientRequest>(url, shared_from_this());
}

std::shared_ptr<IHttpServer> NetworkManagerComponent::CreateHttpServer(
    uint32_t port, const std::string &socketPath) {
  auto server = std::make_shared<HttpServer>(socketPath);
  server->Listen(port);
  return server;
}

std::shared_ptr<IWebSocket> NetworkManagerComponent::CreateWebSocket(
    const std::string &url) {
  auto socket = std::make_shared<WebSocketClient>(
      url, shared_from_this(), _options.WebSocketClientReconnectInterval);
  std::lock_guard<std::mutex> lock(_webSocketMutex);
  _pendingWebSockets.push_back(std::make_pair(socket, NowMs()));
  return socket;
}

std::shared_ptr<HttpClientPendingRequest> NetworkManagerComponent::SendRequest(
    HttpMessage &request,
    std::function<void(std::shared_ptr<HttpMessage> &response)> handler) {
  const bool validProtocol = CaseInsensitiveStartsWith(request.Url, S_HTTP) ||
                             CaseInsensitiveStartsWith(request.Url, S_HTTPS);
  auto pending = std::make_shared<HttpClientPendingRequest>(request, handler);
  if (!validProtocol) {
    HttpMessage response = {.Error = "Invalid URL protocol"};
    pending->SetResponse(HttpRequestState::Error, response);
    return pending;
  }
  {
    std::lock_guard<std::mutex> lock(_requestMutex);
    _newRequests.push_back(pending);
  }
  Wake();
  return pending;
}

void NetworkManagerComponent::ScheduleWebSocketConnect(
    std::weak_ptr<WebSocketClient> socket, size_t when) {
  std::lock_guard<std::mutex> lock(_webSocketMutex);
  _pendingWebSockets.push_back(std::make_pair(socket, when));
}

void NetworkManagerComponent::Poll() {
  while (_running.load()) {
    // start any newly queued requests
    {
      std::deque<std::shared_ptr<HttpClientPendingRequest>> newRequests;
      {
        std::lock_guard<std::mutex> lock(_requestMutex);
        newRequests.swap(_newRequests);
      }
      for (auto &pending : newRequests) {
        if (pending->GetRequestState() == HttpRequestState::Cancelled) {
          continue;
        }
        if (pending->CreateConnection(_multi, _options)) {
          _activeRequests.emplace(pending->Connection(), pending);
        }
      }
    }

    // close the connections of any cancelled requests to ensure
    // they are not processed any further
    for (auto it = _activeRequests.begin(); it != _activeRequests.end();) {
      if (it->second->GetRequestState() == HttpRequestState::Cancelled) {
        it->second->ReleaseConnection(_multi);
        it = _activeRequests.erase(it);
      } else {
        ++it;
      }
    }

    // begin connecting any pending websockets that are due
    {
      std::vector<std::shared_ptr<WebSocketClient>> due;
      {
        std::lock_guard<std::mutex> lock(_webSocketMutex);
        const auto now = NowMs();
        auto it = _pendingWebSockets.begin();
        while (it != _pendingWebSockets.end()) {
          if (now >= it->second) {
            if (auto socket = it->first.lock()) {
              due.push_back(socket);
            }
            it = _pendingWebSockets.erase(it);
          } else {
            ++it;
          }
        }
      }
      for (auto &socket : due) {
        if (socket->Connect(_multi)) {
          _activeWebSockets.push_back(socket.get());
        }
      }
    }

    int active = 0;
    curl_multi_perform(_multi, &active);

    // handle any completed requests or websocket handshakes
    CURLMsg *msg = nullptr;
    int remaining = 0;
    while ((msg = curl_multi_info_read(_multi, &remaining))) {
      if (msg->msg != CURLMSG_DONE) {
        continue;
      }
      CURL *easy = msg->easy_handle;
      const CURLcode result = msg->data.result;
      const auto found = _activeRequests.find(easy);
      if (found != _activeRequests.end()) {
        auto pending = found->second;
        pending->CompleteConnection(result);
        pending->ReleaseConnection(_multi);
        _activeRequests.erase(found);
      } else {
        for (const auto socket : _activeWebSockets) {
          if (socket->Connection() == easy) {
            socket->CompleteConnection(_multi, result);
            break;
          }
        }
      }
    }

    // pump open websocket connections
    for (auto it = _activeWebSockets.begin();
         it != _activeWebSockets.end();) {
      if ((*it)->Pump(_multi) == WebSocketClient::PumpResult::Closed) {
        // after a socket transitions to closed the poll thread must not
        // access it again as it may be destroyed at any time
        it = _activeWebSockets.erase(it);
      } else {
        ++it;
      }
    }

    curl_multi_poll(_multi, nullptr, 0, 16, nullptr);
  }

  // shutting down - cancel all queued and in-flight requests
  {
    std::lock_guard<std::mutex> lock(_requestMutex);
    for (auto &pending : _newRequests) {
      HttpMessage response = {.Code = -1, .Error = S_CANCELLED};
      pending->SetResponse(HttpRequestState::Cancelled, response);
    }
    _newRequests.clear();
  }
  for (auto &pending : _activeRequests) {
    HttpMessage response = {.Code = -1, .Error = S_CANCELLED};
    pending.second->SetResponse(HttpRequestState::Cancelled, response);
    pending.second->ReleaseConnection(_multi);
  }
  _activeRequests.clear();

  // close any active websockets
  for (const auto socket : _activeWebSockets) {
    socket->Shutdown(_multi);
  }
  _activeWebSockets.clear();
}

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
