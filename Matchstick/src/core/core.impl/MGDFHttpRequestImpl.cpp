#include "StdAfx.h"

#include "MGDFHttpRequestImpl.hpp"

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
                                 std::shared_ptr<HttpClient> &client)
    : _request(std::make_shared<HttpRequest>(url)), _client(client) {}

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

IMGDFHttpRequest *HttpRequestImpl::Send() {
  _client->SendRequest(_request);
  _client.reset();
  return this;
}

void HttpRequestImpl::Cancel() { _request->Cancel(); }

BOOL HttpRequestImpl::GetResponse(IMGDFHttpResponse **response) {
  std::shared_ptr<HttpResponse> r;
  if (_request->GetResponse(r)) {
    auto wrapper = MakeComFromPtr<IMGDFHttpResponse>(new HttpResponseImpl(r));
    wrapper.AddRawRef(response);
    return TRUE;
  }
  return FALSE;
}

}  // namespace core
}  // namespace MGDF
