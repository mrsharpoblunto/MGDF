#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <deque>

#include "../common/MGDFHttpClient.hpp"
#include "../common/MGDFWebSocket.hpp"

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

class HttpRequestGroupImpl : public ComBase<IMGDFHttpRequestGroup> {
 public:
  HttpRequestGroupImpl() : Group(std::make_shared<HttpRequestGroup>()) {}
  virtual ~HttpRequestGroupImpl() {}

  void *__stdcall GetResponse(IMGDFHttpResponse **response) final;

  const std::shared_ptr<HttpRequestGroup> Group;
};

class HttpResponseImpl : public ComBase<IMGDFHttpResponse> {
 public:
  HttpResponseImpl(const std::shared_ptr<HttpResponse> &response);
  virtual ~HttpResponseImpl(void) {}

  INT32 __stdcall GetCode(void) final;
  const small *__stdcall GetHeader(const small *name) final;
  const small *__stdcall GetError(void) final;
  UINT64 __stdcall GetBodyLength(void) final;
  const small *__stdcall GetBody(void) final;

 private:
  std::shared_ptr<HttpResponse> _response;
};

class HttpRequestImpl : public ComBase<IMGDFHttpRequest> {
 public:
  HttpRequestImpl(const std::string &url,
                  const std::shared_ptr<HttpClient> &client);
  HttpRequestImpl(const std::shared_ptr<HttpRequest> &request);
  virtual ~HttpRequestImpl(void) {}

  IMGDFHttpRequest *__stdcall SetHeader(const small *name,
                                        const small *value) final;
  IMGDFHttpRequest *__stdcall SetMethod(const small *method) final;
  IMGDFHttpRequest *__stdcall SetBody(const small *body,
                                      UINT64 bodyLength) final;
  void *__stdcall Send(IMGDFHttpRequestGroup *group) final;
  void __stdcall Cancel() final;
  BOOL __stdcall GetResponse(IMGDFHttpResponse **response) final;

 private:
  std::shared_ptr<HttpClient> _client;
  std::shared_ptr<HttpRequest> _request;
};

class WebSocketServerImpl : public WebSocketServer,
                            public ComBase<IMGDFWebSocketServer> {
 public:
  WebSocketServerImpl(uint32_t port);
  virtual ~WebSocketServerImpl(void) {}
  BOOL __stdcall ClientConnected(IMGDFWebSocket **connection) final;
  BOOL __stdcall Listening() final { return WebSocketServer::Listening(); }
  void OnConnected(std::shared_ptr<WebSocketConnection> &connection) final;

 private:
  std::mutex _mutex;
  std::deque<std::shared_ptr<WebSocketConnection>> _newConnections;
};

}  // namespace core
}  // namespace MGDF
