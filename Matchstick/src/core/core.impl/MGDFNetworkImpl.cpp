#include "StdAfx.h"

#include "MGDFNetworkImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

WebSocketImpl::WebSocketImpl(std::shared_ptr<network::IWebSocket> socket)
    : _socket(socket), _binary(false) {
  _socket->OnReceive([this](std::span<const char> &message, bool binary) {
    // TODO need to have a buffer of buffers...
    std::lock_guard<std::mutex> lock(_mutex);
    _binary = binary;
    _buffer.resize(message.size());
    memcpy_s(_buffer.data(), _buffer.size(), message.data(), message.size());
  });
}

void WebSocketImpl::Send(void *data, UINT64 len, BOOL binary) {
  _socket->Send(data, len, binary);
}

BOOL WebSocketImpl::CanRecieve(UINT64 *len) {
  std::lock_guard<std::mutex> lock(_mutex);
  *len = _buffer.size();
  return _buffer.size() > 0;
}

HRESULT WebSocketImpl::Receive(void *message, UINT64 len, BOOL *binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (len >= _buffer.size()) {
    *binary = _binary;
    memcpy_s(message, _buffer.size(), _buffer.data(), _buffer.size());
    _buffer.clear();
    return S_OK;
  }
  return E_FAIL;
}

HRESULT WebSocketImpl::GetConnectionStatus(
    MGDFWebSocketConnectionStatus *status) {
  std::string lastError;
  const auto state = _socket->GetConnectionState(lastError);
  if (status->LastErrorLength >= lastError.size()) {
    status->State = state;
    memcpy_s(status->LastError, lastError.size(), lastError.c_str(),
             lastError.size());
    return S_OK;
  }
  return E_FAIL;
}
HttpClientResponseImpl::HttpClientResponseImpl(
    const std::shared_ptr<network::HttpMessage> &response)
    : _response(response) {}

INT32 HttpClientResponseImpl::GetResponseCode() { return _response->Code; }

const small *HttpClientResponseImpl::GetResponseHeader(const small *name) {
  auto found = _response->Headers.find(name);
  if (found != _response->Headers.end()) {
    return found->second.c_str();
  } else {
    return nullptr;
  }
}

const small *HttpClientResponseImpl::GetResponseError(void) {
  return _response->Error.size() ? _response->Error.c_str() : nullptr;
}

UINT64 HttpClientResponseImpl::GetResponseBodyLength(void) {
  return _response->Body.size();
}

const small *HttpClientResponseImpl::GetResponseBody(void) {
  return _response->Body.data();
}

void HttpClientRequestGroupImpl::OnRequest(
    void *key, std::shared_ptr<network::HttpMessage> &message) {
  std::lock_guard<std::mutex> lock(_mutex);
  _responses.insert(std::make_pair(key, message));
}

void *HttpClientRequestGroupImpl::GetResponse(
    IMGDFHttpClientResponse **responseOut) {
  std::unique_lock<std::mutex> lock(_mutex);
  if (!_responses.size()) {
    return nullptr;
  }
  auto r = _responses.begin();
  _responses.erase(r);
  lock.unlock();

  auto responseWrapper = MakeCom<HttpClientResponseImpl>(r->second);
  responseWrapper.AddRawRef(responseOut);
  return r->first;
}

HttpClientRequestImpl::HttpClientRequestImpl(
    std::unique_ptr<network::IHttpClientRequest> &request)
    : _request(std::move(request)) {}

IMGDFHttpClientRequest *HttpClientRequestImpl::SetRequestHeader(
    const small *name, const small *value) {
  _request->SetRequestHeader(name, value);
  return this;
}

IMGDFHttpClientRequest *HttpClientRequestImpl::SetRequestMethod(
    const small *method) {
  _request->SetRequestMethod(method);
  return this;
}

IMGDFHttpClientRequest *HttpClientRequestImpl::SetRequestBody(
    const small *body, UINT64 bodyLength) {
  _request->SetRequestBody(body, bodyLength, false);
  return this;
}

void *HttpClientRequestImpl::SendRequest(IMGDFHttpClientRequestGroup *group) {
  if (group) {
    _group = MGDF::ComObject<IMGDFHttpClientRequestGroup>(group, true);
  }

  _pending = _request->SendRequest(
      [this](std::shared_ptr<network::HttpMessage> &response) {
        if (_group && _pending) {
          auto group = dynamic_cast<HttpClientRequestGroupImpl *>(_group.Get());
          _ASSERTE(group);
          group->OnRequest(_pending.get(), response);
        }
      });
  return _pending.get();
}

void HttpClientRequestImpl::CancelRequest() {
  if (_pending) {
    _pending->CancelRequest();
  }
}

BOOL HttpClientRequestImpl::GetResponse(IMGDFHttpClientResponse **response) {
  std::shared_ptr<network::HttpMessage> r;
  if (_pending && _pending->GetResponse(r)) {
    auto wrapper = MakeCom<HttpClientResponseImpl>(r);
    wrapper.AddRawRef(response);
    return TRUE;
  }
  return FALSE;
}

HttpServerRequestImpl::HttpServerRequestImpl(
    const std::shared_ptr<network::IHttpServerRequest> &request)
    : _request(request) {}

const small *__stdcall HttpServerRequestImpl::GetRequestHeader(
    const small *name) {
  if (_request->HasRequestHeader(name)) {
    return _request->GetRequestHeader(name).c_str();
  } else {
    return nullptr;
  }
}
const small *__stdcall HttpServerRequestImpl::GetRequestMethod() {
  return _request->GetRequestMethod().c_str();
}
const small *HttpServerRequestImpl::GetRequestBody() {
  return _request->GetRequestBody().c_str();
}
UINT64 HttpServerRequestImpl::GetRequestBodyLength() {
  return _request->GetRequestBody().size();
}

IMGDFHttpServerRequest *HttpServerRequestImpl::SetResponseCode(INT32 code) {
  _request->SetResponseCode(code);
  return this;
}

IMGDFHttpServerRequest *HttpServerRequestImpl::SetResponseHeader(
    const small *name, const small *value) {
  _request->SetResponseHeader(name, value);
  return this;
}

IMGDFHttpServerRequest *HttpServerRequestImpl::SetResponseBody(
    const small *body, UINT64 bodyLength) {
  _request->SetResponseBody(body, bodyLength);
  return this;
}

void HttpServerRequestImpl::SendResponse() { _request->SendResponse(); }

WebServerImpl::WebServerImpl(std::shared_ptr<network::IHttpServer> server)
    : _server(server) {
  server->OnHttpRequest(
      [this](std::shared_ptr<network::IHttpServerRequest> request) {
        std::lock_guard<std::mutex> lock(_mutex);
        _requests.push_back(MGDFWebServerRequest{
            .WebSocket = nullptr,
            .HttpRequest = MakeCom<HttpServerRequestImpl>(request),
        });
      });
  server->OnWebSocketRequest(
      [this](std::shared_ptr<network::IWebSocket> socket) {
        std::lock_guard<std::mutex> lock(_mutex);
        _requests.push_back(MGDFWebServerRequest{
            .WebSocket = MakeCom<WebSocketImpl>(socket),
            .HttpRequest = nullptr,
        });
      });
}

BOOL WebServerImpl::RequestRecieved(MGDFWebServerRequest *request) {
  std::unique_lock<std::mutex> lock(_mutex);
  if (!_requests.size()) {
    return FALSE;
  }
  auto &r = _requests.front();
  _requests.pop_front();
  request->HttpRequest = r.HttpRequest;
  request->WebSocket = r.WebSocket;
  return true;
}

}  // namespace core
}  // namespace MGDF
