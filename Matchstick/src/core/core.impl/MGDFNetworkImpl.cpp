#include "StdAfx.h"

#include "MGDFNetworkImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

HttpResponseImpl::HttpResponseImpl(
    const std::shared_ptr<HttpResponse> &response)
    : _response(response) {}

INT32 HttpResponseImpl::GetCode() { return _response->Code; }

const small *HttpResponseImpl::GetHeader(const small *name) {
  auto found = _response->Headers.find(name);
  if (found != _response->Headers.end()) {
    return found->second.c_str();
  } else {
    return nullptr;
  }
}

const small *HttpResponseImpl::GetError(void) {
  return _response->Error.size() ? _response->Error.c_str() : nullptr;
}

UINT64 HttpResponseImpl::GetBodyLength(void) { return _response->Body.size(); }

const small *HttpResponseImpl::GetBody(void) { return _response->Body.data(); }

HttpRequestImpl::HttpRequestImpl(const std::string &url,
                                 const std::shared_ptr<HttpClient> &client)
    : _request(std::make_shared<HttpRequest>(url)), _client(client) {}

HttpRequestImpl::HttpRequestImpl(const std::shared_ptr<HttpRequest> &request)
    : _request(request) {}

IMGDFHttpRequest *HttpRequestImpl::SetHeader(const small *name,
                                             const small *value) {
  _request->SetHeader(name, value);
  return this;
}

IMGDFHttpRequest *HttpRequestImpl::SetMethod(const small *method) {
  _request->SetMethod(method);
  return this;
}

IMGDFHttpRequest *HttpRequestImpl::SetBody(const small *body,
                                           UINT64 bodyLength) {
  _request->SetBody(body, bodyLength);
  return this;
}

void *HttpRequestImpl::Send(IMGDFHttpRequestGroup *group) {
  _client->SendRequest(_request,
                       dynamic_cast<HttpRequestGroupImpl *>(group)->Group);
  _client.reset();
  return _request.get();
}

void HttpRequestImpl::Cancel() { _request->Cancel(); }

BOOL HttpRequestImpl::GetResponse(IMGDFHttpResponse **response) {
  std::shared_ptr<HttpResponse> r;
  if (_request->GetResponse(r)) {
    auto wrapper = MakeCom<HttpResponseImpl>(r);
    wrapper.AddRawRef(response);
    return TRUE;
  }
  return FALSE;
}

void *HttpRequestGroupImpl::GetResponse(IMGDFHttpResponse **responseOut) {
  std::shared_ptr<HttpResponse> response;
  auto request = Group->GetResponse(response);
  if (request) {
    auto responseWrapper = MakeCom<HttpResponseImpl>(response);
    responseWrapper.AddRawRef(responseOut);
    return request.get();
  }
  return nullptr;
}

WebSocketServerImpl::WebSocketServerImpl(uint32_t port) {
  Listen("0.0.0.0:" + std::to_string(port));
}

void WebSocketServerImpl::OnConnected(
    std::shared_ptr<WebSocketConnection> &connection) {
  std::lock_guard<std::mutex> lock(_mutex);
  _newConnections.push_back(connection);
}

BOOL WebSocketServerImpl::ClientConnected(IMGDFWebSocket **connection) {
  std::unique_lock<std::mutex> lock(_mutex);
  if (!_newConnections.size()) return FALSE;
  auto c = _newConnections.front();
  _newConnections.pop_front();
  lock.unlock();

  auto com = MakeCom<WebSocketImpl<WebSocketConnection>>(c);
  com.AddRawRef(connection);
  return TRUE;
}

}  // namespace core
}  // namespace MGDF
