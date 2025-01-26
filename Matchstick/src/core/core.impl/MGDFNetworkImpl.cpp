#include "StdAfx.h"

#include "MGDFNetworkImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

WebSocketImpl::WebSocketImpl(std::shared_ptr<network::IWebSocket> socket)
    : _socket(socket) {
  _socket->OnReceive([this](std::span<const char> &message, bool binary) {
    std::lock_guard<std::mutex> lock(_mutex);
    _buffers.emplace_back(message.begin(), message.end());
    _binaryFlags.push_back(binary);
  });
}

void WebSocketImpl::Send(void *data, UINT64 len, BOOL binary) {
  _socket->Send(data, len, binary);
}

BOOL WebSocketImpl::CanRecieve(UINT64 *len) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_buffers.size() == 0) {
    return false;
  }
  const auto front = _buffers.front();
  *len = front.size();
  return front.size() > 0;
}

HRESULT WebSocketImpl::Receive(void *message, UINT64 len, BOOL *binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_buffers.size() == 0) {
    return E_FAIL;
  }
  const auto front = _buffers.front();
  if (len >= front.size()) {
    *binary = _binaryFlags.front();
    memcpy_s(message, front.size(), front.data(), front.size());
    _buffers.pop_front();
    _binaryFlags.pop_front();
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
  } else {
    status->LastErrorLength = lastError.size();
    return E_FAIL;
  }
}
HttpClientResponseImpl::HttpClientResponseImpl(
    const std::shared_ptr<network::HttpMessage> &response)
    : _response(response) {}

INT32 HttpClientResponseImpl::GetResponseCode() { return _response->Code; }

HRESULT HttpClientResponseImpl::GetResponseHeader(const small *name,
                                                  small *value,
                                                  UINT64 *length) {
  auto found = _response->Headers.find(name);
  if (found != _response->Headers.end()) {
    if (*length >= found->second.size()) {
      memcpy_s(value, found->second.size(), found->second.c_str(),
               found->second.size());
      return S_OK;
    } else {
      *length = found->second.size();
      return E_FAIL;
    }
  } else {
    return E_NOT_SET;
  }
}

HRESULT HttpClientResponseImpl::GetResponseError(small *error, UINT64 *length) {
  if (*length >= _response->Error.size()) {
    memcpy_s(error, _response->Error.size(), _response->Error.c_str(),
             _response->Error.size());
    return S_OK;
  } else {
    *length = _response->Error.size();
    return E_FAIL;
  }
}

HRESULT HttpClientResponseImpl::GetResponseBody(small *body, UINT64 *length) {
  if (*length >= _response->Body.size()) {
    memcpy_s(body, _response->Body.size(), _response->Body.data(),
             _response->Body.size());
    return S_OK;
  } else {
    *length = _response->Body.size();
    return E_FAIL;
  }
}

void HttpClientRequestGroupImpl::OnRequest(
    void *key, ComObject<IMGDFHttpClientResponse> &response) {
  std::lock_guard<std::mutex> lock(_mutex);
  _responses.insert(std::make_pair(key, response));
}

void *HttpClientRequestGroupImpl::GetResponse(
    IMGDFHttpClientResponse **responseOut) {
  std::unique_lock<std::mutex> lock(_mutex);
  if (!_responses.size()) {
    return nullptr;
  }
  auto pair = _responses.begin();
  auto r = pair->second;
  r.AddRawRef(responseOut);
  auto key = pair->first;
  _responses.erase(pair);
  lock.unlock();

  return key;
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
  if (!_pending) {
    if (group) {
      _group = MGDF::ComObject<IMGDFHttpClientRequestGroup>(group, true);
    }

    // keep this alive until the pending request is complete
    // once the pending response handler is called it will get
    // released, which will clear out this reference kept alive
    // in the handler closure
    ComObject<HttpClientRequestImpl> self(this, true);

    _pending = _request->SendRequest(
        [self](std::shared_ptr<network::HttpMessage> &response) {
          std::unique_lock<std::mutex> lock(self->_mutex);
          if (self->_group) {
            if (!self->_response) {
              self->_response = MakeCom<HttpClientResponseImpl>(response);
            }
            _ASSERTE(self->_pending);
            const auto key = self->_pending.get();
            self->_pending.reset();
            lock.unlock();

            const auto group =
                dynamic_cast<HttpClientRequestGroupImpl *>(self->_group.Get());
            _ASSERTE(group);
            group->OnRequest(key, self->_response);
          }
        });
  }
  return group ? _pending.get() : nullptr;
}

void HttpClientRequestImpl::CancelRequest() {
  if (_pending) {
    _pending->CancelRequest();
  }
}

HttpClientRequestImpl::~HttpClientRequestImpl() {}

BOOL HttpClientRequestImpl::GetResponse(IMGDFHttpClientResponse **response) {
  std::unique_lock<std::mutex> lock(_mutex);
  ComObject<IMGDFHttpClientResponse> r;
  if (_response) {
    r = _response;
  } else {
    std::shared_ptr<network::HttpMessage> newResponse;
    if (_pending && _pending->GetResponse(newResponse)) {
      if (!_group) {
        _pending.reset();
      }
      if (!_response) {
        _response = MakeCom<HttpClientResponseImpl>(newResponse);
      }
      r = _response;
    }
  }
  lock.unlock();
  if (r) {
    r.AddRawRef(response);
    return TRUE;
  }
  return FALSE;
}

HttpServerRequestImpl::HttpServerRequestImpl(
    const std::shared_ptr<network::IHttpServerRequest> &request)
    : _request(request) {}

HRESULT HttpServerRequestImpl::GetRequestHeader(const small *name, small *value,
                                                UINT64 *length) {
  if (_request->HasRequestHeader(name)) {
    auto &hdr = _request->GetRequestHeader(name);
    if (*length >= hdr.size()) {
      memcpy_s(value, hdr.size(), hdr.c_str(), hdr.size());
      return S_OK;
    } else {
      *length = hdr.size();
      return E_FAIL;
    }
  } else {
    return E_NOT_SET;
  }
}

HRESULT HttpServerRequestImpl::GetRequestPath(small *path, UINT64 *length) {
  auto &m = _request->GetRequestPath();
  if (*length >= m.size()) {
    memcpy_s(path, m.size(), m.c_str(), m.size());
    return S_OK;
  } else {
    *length = m.size();
    return E_FAIL;
  }
}

HRESULT HttpServerRequestImpl::GetRequestMethod(small *method, UINT64 *length) {
  auto &m = _request->GetRequestMethod();
  if (*length >= m.size()) {
    memcpy_s(method, m.size(), m.c_str(), m.size());
    return S_OK;
  } else {
    *length = m.size();
    return E_FAIL;
  }
}

HRESULT HttpServerRequestImpl::GetRequestBody(small *body, UINT64 *length) {
  auto &b = _request->GetRequestBody();
  if (*length >= b.size()) {
    memcpy_s(body, b.size(), b.data(), b.size());
    return S_OK;
  } else {
    *length = b.size();
    return E_FAIL;
  }
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
  _server->OnHttpRequest(
      [this](std::shared_ptr<network::IHttpServerRequest> request) {
        std::lock_guard<std::mutex> lock(_mutex);
        _requests.push_back(WebServerImplRequest{
            .WebSocket = ComObject<WebSocketImpl>(),
            .HttpRequest = MakeCom<HttpServerRequestImpl>(request),
        });
      });
  _server->OnWebSocketRequest(
      [this](std::shared_ptr<network::IWebSocket> socket) {
        std::lock_guard<std::mutex> lock(_mutex);
        _requests.push_back(WebServerImplRequest{
            .WebSocket = MakeCom<WebSocketImpl>(socket),
            .HttpRequest = ComObject<HttpServerRequestImpl>(),
        });
      });
}

BOOL WebServerImpl::RequestRecieved(MGDFWebServerRequest *request) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (!_requests.size()) {
    return FALSE;
  }
  auto &r = _requests.front();
  r.HttpRequest.AddRawRef(&request->HttpRequest);
  r.WebSocket.AddRawRef(&request->WebSocket);
  _requests.pop_front();
  return TRUE;
}

BOOL WebServerImpl::Listening() { return _server->Listening(); }

}  // namespace core
}  // namespace MGDF
