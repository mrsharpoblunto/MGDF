#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

#include "../common/MGDFHttpClient.hpp"

namespace MGDF {
namespace core {

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
  HttpRequestImpl(const std::string &url, std::shared_ptr<HttpClient> &client);
  virtual ~HttpRequestImpl(void) {}

  IMGDFHttpRequest *__stdcall SetHeader(const small *name,
                                        const small *value) final;
  IMGDFHttpRequest *__stdcall SetMethod(const small *method) final;
  IMGDFHttpRequest *__stdcall SetBody(const small *body,
                                      UINT64 bodyLength) final;
  IMGDFHttpRequest *__stdcall Send() final;
  void __stdcall Cancel() final;
  BOOL __stdcall GetResponse(IMGDFHttpResponse **response) final;

 private:
  std::shared_ptr<HttpClient> _client;
  std::shared_ptr<HttpRequest> _request;
};

}  // namespace core
}  // namespace MGDF