#include "StdAfx.h"

#include "MGDFNetworkImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

WebSocketImpl::WebSocketImpl(const std::string &url)
    : _socket(new WebSocketClient(url)) {}

void WebSocketImpl::Send(void *data, UINT64 len, BOOL binary) {
  _socket->Send(data, len, binary);
}

BOOL WebSocketImpl::CanRecieve(UINT64 *len) {
  if (!_buffer.size()) {
    _socket->Receive(_buffer);
  }
  *len = _buffer.size();
  return _buffer.size() > 0;
}

HRESULT WebSocketImpl::Receive(void *message, UINT64 len) {
  if (!_buffer.size()) {
    _socket->Receive(_buffer);
  }

  if (len >= _buffer.size()) {
    memcpy_s(message, _buffer.size(), _buffer.data(), _buffer.size());
    _buffer.clear();
    return S_OK;
  }
  return E_FAIL;
}

HRESULT WebSocketImpl::GetConnectionStatus(
    MGDFWebSocketConnectionStatus *status) {
  std::string lastError;
  auto state = _socket->GetConnectionState(lastError);
  if (status->LastErrorLength >= lastError.size()) {
    status->State = state;
    memcpy_s(status->LastError, lastError.size(), lastError.c_str(),
             lastError.size());
    return S_OK;
  }
  return E_FAIL;
}

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

}  // namespace core
}  // namespace MGDF
