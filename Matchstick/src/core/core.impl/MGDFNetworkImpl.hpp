#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "../common/MGDFHttpClient.hpp"
#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFWebSocket.hpp"

namespace MGDF {
namespace core {

class WebSocketImpl : public ComBase<IMGDFWebSocket> {
 public:
  WebSocketImpl(const std::string &url);
  virtual ~WebSocketImpl(void) {}

  void __stdcall Send(void *data, UINT64 len, BOOL binary) final;
  BOOL __stdcall CanRecieve(UINT64 *len) final;
  HRESULT __stdcall Receive(void *message, UINT64 len) final;
  HRESULT __stdcall GetConnectionStatus(
      MGDFWebSocketConnectionStatus *status) final;

 private:
  std::shared_ptr<WebSocketClient> _socket;
  std::vector<char> _buffer;
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

}  // namespace core
}  // namespace MGDF
