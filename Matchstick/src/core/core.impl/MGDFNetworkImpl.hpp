#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <deque>

#include "../network/MGDFNetworkManagerComponent.hpp"

namespace MGDF {
namespace core {

template <typename TSocket>
class WebSocketImpl : public ComBase<IMGDFWebSocket> {
 public:
  WebSocketImpl(std::shared_ptr<TSocket> socket) : _socket(socket) {}
  virtual ~WebSocketImpl(void) {}

  void __stdcall Send(void *data, UINT64 len, BOOL binary) final {
    _socket->Send(data, len, binary);
  }

  BOOL __stdcall CanRecieve(UINT64 *len) final {
    if (!_buffer.size()) {
      _socket->Receive(_buffer, _binary);
    }
    *len = _buffer.size();
    return _buffer.size() > 0;
  }

  HRESULT __stdcall Receive(void *message, UINT64 len, BOOL *binary) final {
    if (!_buffer.size()) {
      _socket->Receive(_buffer, _binary);
    }

    if (len >= _buffer.size()) {
      *binary = _binary;
      memcpy_s(message, _buffer.size(), _buffer.data(), _buffer.size());
      _buffer.clear();
      return S_OK;
    }
    return E_FAIL;
  }

  HRESULT __stdcall GetConnectionStatus(
      MGDFWebSocketConnectionStatus *status) final {
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

 private:
  std::shared_ptr<TSocket> _socket;
  std::vector<char> _buffer;
  bool _binary;
};

class HttpRequestImpl;

class HttpClientRequestGroupImpl : public ComBase<IMGDFHttpClientRequestGroup> {
 public:
  HttpClientRequestGroupImpl()
      : Group(std::make_shared<HttpClientRequestGroup>()) {}
  virtual ~HttpClientRequestGroupImpl() {}

  void *__stdcall GetResponse(IMGDFHttpClientResponse **response) final;

  const std::shared_ptr<HttpClientRequestGroup> Group;
};

class HttpClientResponseImpl : public ComBase<IMGDFHttpClientResponse> {
 public:
  HttpClientResponseImpl(
      const std::shared_ptr<network::HttpResponse> &response);
  virtual ~HttpClientResponseImpl(void) {}

  INT32 __stdcall GetResponseCode(void) final;
  const small *__stdcall GetResponseHeader(const small *name) final;
  const small *__stdcall GetResponseError(void) final;
  UINT64 __stdcall GetResponseBodyLength(void) final;
  const small *__stdcall GetResponseBody(void) final;

 private:
  std::shared_ptr<network::HttpResponse> _response;
};

class HttpClientRequestImpl : public ComBase<IMGDFHttpClientRequest> {
 public:
  HttpClientRequestImpl(const std::string &url,
                        const std::shared_ptr<HttpClient> &client);
  HttpClientRequestImpl(const std::shared_ptr<HttpClientRequest> &request);
  virtual ~HttpClientRequestImpl(void) {}

  IMGDFHttpClientRequest *__stdcall SetRequestHeader(const small *name,
                                                     const small *value) final;
  IMGDFHttpClientRequest *__stdcall SetRequestMethod(const small *method) final;
  IMGDFHttpClientRequest *__stdcall SetRequestBody(const small *body,
                                                   UINT64 bodyLength) final;
  void *__stdcall SendRequest(IMGDFHttpClientRequestGroup *group) final;
  void __stdcall CancelRequest() final;
  BOOL __stdcall GetResponse(IMGDFHttpClientResponse **response) final;

 private:
  std::shared_ptr<HttpClient> _client;
  std::shared_ptr<HttpClientRequest> _request;
};

class HttpServerRequestImpl : public ComBase<IMGDFHttpServerRequest> {
 public:
  HttpServerRequestImpl(const std::shared_ptr<HttpServerRequest> &request);
  virtual ~HttpServerRequestImpl(void) {}

  const small *__stdcall GetRequestHeader(const small *name) final;
  const small *__stdcall GetRequestMethod() final;
  const small *__stdcall GetRequestBody() final;
  UINT64 __stdcall GetRequestBodyLength() final;

  IMGDFHttpServerRequest *__stdcall SetResponseCode(INT32 code) final;
  IMGDFHttpServerRequest *__stdcall SetResponseHeader(const small *name,
                                                      const small *value) final;
  IMGDFHttpServerRequest *__stdcall SetResponseMethod(
      const small *method) final;
  IMGDFHttpServerRequest *__stdcall SetResponseBody(const small *body,
                                                    UINT64 bodyLength) final;
  void __stdcall SendResponse() final;

 private:
  std::shared_ptr<HttpServerRequest> _request;
};

class WebServerImpl : public HttpServer, public ComBase<IMGDFWebServer> {
 public:
  WebServerImpl(std::shared_ptr<NetworkEventLoop> &eventLoop, uint32_t port,
                const std::string &socketPath);
  virtual ~WebServerImpl(void) {}
  BOOL __stdcall RequestRecieved(MGDFWebServerRequest *request) final;
  BOOL __stdcall Listening() final { return HttpServer::Listening(); }

  void OnRequest(std::shared_ptr<HttpServerRequest> &request) final;
  void OnSocketRequest(
      std::shared_ptr<WebSocketServerConnection> &socket) final;

 private:
  std::mutex _mutex;
  std::deque<ComObject<WebSocketImpl<WebSocketServerConnection>>> _sockets;
  std::deque<ComObject<HttpServerRequestImpl>> _requests;
};

}  // namespace core
}  // namespace MGDF
