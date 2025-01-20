#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
#include <deque>
#include <mutex>

#include "../network/MGDFNetworkManagerComponent.hpp"

namespace MGDF {
namespace core {

class WebSocketImpl : public ComBase<IMGDFWebSocket> {
 public:
  WebSocketImpl(std::shared_ptr<network::IWebSocket> socket);
  virtual ~WebSocketImpl(void) {}

  void __stdcall Send(void *data, UINT64 len, BOOL binary) final;

  BOOL __stdcall CanRecieve(UINT64 *len) final;
  HRESULT __stdcall Receive(void *message, UINT64 len, BOOL *binary) final;
  HRESULT __stdcall GetConnectionStatus(
      MGDFWebSocketConnectionStatus *status) final;

 private:
  std::mutex _mutex;
  std::shared_ptr<network::IWebSocket> _socket;
  std::deque<std::vector<char>> _buffers;
  std::deque<bool> _binaryFlags;
};

class HttpRequestImpl;

class HttpClientRequestGroupImpl : public ComBase<IMGDFHttpClientRequestGroup> {
 public:
  HttpClientRequestGroupImpl() {}
  virtual ~HttpClientRequestGroupImpl() {}

  void *__stdcall GetResponse(IMGDFHttpClientResponse **response) final;

  void OnRequest(void *key, ComObject<IMGDFHttpClientResponse> &response);

 private:
  std::mutex _mutex;
  std::unordered_map<void *, ComObject<IMGDFHttpClientResponse>> _responses;
};

class HttpClientResponseImpl : public ComBase<IMGDFHttpClientResponse> {
 public:
  HttpClientResponseImpl(const std::shared_ptr<network::HttpMessage> &response);
  virtual ~HttpClientResponseImpl(void) {}

  INT32 __stdcall GetResponseCode(void) final;
  HRESULT __stdcall GetResponseHeader(const small *name, small *value,
                                      UINT64 *length) final;
  HRESULT __stdcall GetResponseError(small *error, UINT64 *length) final;
  HRESULT __stdcall GetResponseBody(small *body, UINT64 *length) final;

 private:
  std::shared_ptr<network::HttpMessage> _response;
};

class HttpClientRequestImpl : public ComBase<IMGDFHttpClientRequest> {
 public:
  HttpClientRequestImpl(std::unique_ptr<network::IHttpClientRequest> &request);
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
  std::mutex _mutex;
  MGDF::ComObject<IMGDFHttpClientRequestGroup> _group;
  MGDF::ComObject<IMGDFHttpClientResponse> _response;
  std::unique_ptr<network::IHttpClientRequest> _request;
  std::shared_ptr<network::IHttpClientPendingRequest> _pending;
};

class HttpServerRequestImpl : public ComBase<IMGDFHttpServerRequest> {
 public:
  HttpServerRequestImpl(
      const std::shared_ptr<network::IHttpServerRequest> &request);
  virtual ~HttpServerRequestImpl(void) {}

  HRESULT __stdcall GetRequestHeader(const small *name, small *value,
                                     UINT64 *length) final;
  HRESULT __stdcall GetRequestMethod(small *method, UINT64 *length) final;
  HRESULT __stdcall GetRequestBody(small *body, UINT64 *length) final;

  IMGDFHttpServerRequest *__stdcall SetResponseCode(INT32 code) final;
  IMGDFHttpServerRequest *__stdcall SetResponseHeader(const small *name,
                                                      const small *value) final;
  IMGDFHttpServerRequest *__stdcall SetResponseBody(const small *body,
                                                    UINT64 bodyLength) final;
  void __stdcall SendResponse() final;

 private:
  std::shared_ptr<network::IHttpServerRequest> _request;
};

class WebServerImpl : public ComBase<IMGDFWebServer> {
 public:
  WebServerImpl(std::shared_ptr<network::IHttpServer> sever);
  virtual ~WebServerImpl(void) {}
  BOOL __stdcall RequestRecieved(MGDFWebServerRequest *request) final;
  BOOL __stdcall Listening() final;

 private:
  std::mutex _mutex;
  std::deque<MGDFWebServerRequest> _requests;
  std::shared_ptr<network::IHttpServer> _server;
};

}  // namespace core
}  // namespace MGDF
