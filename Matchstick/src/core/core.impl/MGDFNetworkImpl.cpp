#include "StdAfx.h"

#include "MGDFNetworkImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

HttpClientResponseImpl::HttpClientResponseImpl(
    const std::shared_ptr<HttpClientResponse> &response)
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

HttpClientRequestImpl::HttpClientRequestImpl(
    const std::string &url, const std::shared_ptr<HttpClient> &client)
    : _request(std::make_shared<HttpClientRequest>(url)), _client(client) {}

HttpClientRequestImpl::HttpClientRequestImpl(
    const std::shared_ptr<HttpClientRequest> &request)
    : _request(request) {}

IMGDFHttpClientRequest *HttpClientRequestImpl::SetRequestHeader(
    const small *name, const small *value) {
  _request->SetHeader(name, value);
  return this;
}

IMGDFHttpClientRequest *HttpClientRequestImpl::SetRequestMethod(
    const small *method) {
  _request->SetMethod(method);
  return this;
}

IMGDFHttpClientRequest *HttpClientRequestImpl::SetRequestBody(
    const small *body, UINT64 bodyLength) {
  _request->SetBody(body, bodyLength);
  return this;
}

void *HttpClientRequestImpl::SendRequest(IMGDFHttpClientRequestGroup *group) {
  _client->SendRequest(
      _request, dynamic_cast<HttpClientRequestGroupImpl *>(group)->Group);
  _client.reset();
  return _request.get();
}

void HttpClientRequestImpl::CancelRequest() { _request->Cancel(); }

BOOL HttpClientRequestImpl::GetResponse(IMGDFHttpClientResponse **response) {
  std::shared_ptr<HttpClientResponse> r;
  if (_request->GetResponse(r)) {
    auto wrapper = MakeCom<HttpClientResponseImpl>(r);
    wrapper.AddRawRef(response);
    return TRUE;
  }
  return FALSE;
}

void *HttpClientRequestGroupImpl::GetResponse(
    IMGDFHttpClientResponse **responseOut) {
  std::shared_ptr<HttpClientResponse> response;
  auto request = Group->GetResponse(response);
  if (request) {
    auto responseWrapper = MakeCom<HttpClientResponseImpl>(response);
    responseWrapper.AddRawRef(responseOut);
    return request.get();
  }
  return nullptr;
}

HttpServerRequestImpl::HttpServerRequestImpl(
    const std::shared_ptr<HttpServerRequest> &request)
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

IMGDFHttpServerRequest *HttpServerRequestImpl::SetResponseMethod(
    const small *method) {
  _request->SetResponseMethod(method);
  return this;
}

IMGDFHttpServerRequest *HttpServerRequestImpl::SetResponseBody(
    const small *body, UINT64 bodyLength) {
  _request->SetResponseBody(body, bodyLength);
  return this;
}

void HttpServerRequestImpl::SendResponse() { _request->SendResponse(); }

WebServerImpl::WebServerImpl(std::shared_ptr<NetworkEventLoop> &eventLoop,
                             uint32_t port, const std::string &socketPath)
    : HttpServer(eventLoop) {
  Listen(std::to_string(port), socketPath);
}

void WebServerImpl::OnRequest(std::shared_ptr<HttpServerRequest> &request) {
  std::lock_guard<std::mutex> lock(_mutex);
  _requests.push_back(MakeCom<HttpServerRequestImpl>(request));
}
void WebServerImpl::OnSocketRequest(
    std::shared_ptr<WebSocketServerConnection> &socket) {
  std::lock_guard<std::mutex> lock(_mutex);
  _sockets.push_back(MakeCom<WebSocketImpl<WebSocketServerConnection>>(socket));
}

BOOL WebServerImpl::RequestRecieved(MGDFWebServerRequest *request) {
  memset(request, 0, sizeof(MGDFWebServerRequest));
  std::unique_lock<std::mutex> lock(_mutex);
  if (_sockets.size()) {
    auto c = _sockets.front();
    _sockets.pop_front();
    lock.unlock();
    c.AddRawRef(&request->WebSocket);
    return TRUE;
  } else if (_requests.size()) {
    auto r = _requests.front();
    _requests.pop_front();
    lock.unlock();
    r.AddRawRef(&request->HttpRequest);
    return TRUE;
  }
  return FALSE;
}

}  // namespace core
}  // namespace MGDF
