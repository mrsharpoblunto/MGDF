#include "stdafx.h"

#include "MongooseHttpCommon.hpp"

#include <sstream>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

static const std::string S_CONTENT_ENCODING("Content-Encoding");
static const std::string S_ACCEPT_ENCODING("Accept-Encoding");
static const std::string S_CONTENT_LENGTH("Content-Length");
static const std::string S_GZIP("gzip");

namespace MGDF {
namespace core {
namespace network {
namespace mongoose {

void CreateHttpMessage(mg_http_message *hm, HttpMessage &response) {
  response.Code = mg_http_status(hm);
  response.Method = std::string(hm->method.ptr, hm->method.len);
  constexpr const size_t max = sizeof(hm->headers) / sizeof(hm->headers[0]);
  for (size_t i = 0; i < max && hm->headers[i].name.len > 0; i++) {
    mg_str *k = &hm->headers[i].name, *v = &hm->headers[i].value;
    response.Headers.insert(std::make_pair(std::string(k->ptr, k->len),
                                           std::string(v->ptr, v->len)));
  }

  const auto contentEncodingHeader =
      mg_http_get_header(hm, S_CONTENT_ENCODING.c_str());
  if (contentEncodingHeader) {
    if (mg_strcmp(*contentEncodingHeader, mg_stdstr(S_GZIP)) == 0) {
      Resources::DecompressString(hm->body.ptr, hm->body.len, response.Body);
    } else {
      response.Error = "Unsupported Content-Encoding";
      response.Body.clear();
    }
  } else {
    response.Body.resize(hm->body.len);
    memcpy_s(response.Body.data(), response.Body.size(), hm->body.ptr,
             hm->body.len);
  }
}

void SendHttpRequest(mg_connection *c, const std::string &host,
                     const HttpMessage &m) {
  std::vector<char> compressedBody;
  const char *bodyData = m.Body.data();
  size_t bodyLength = m.Body.size();
  bool removeContentEncoding = false;

  if (!m.Body.empty()) {
    auto found = m.Headers.find(S_CONTENT_ENCODING);
    if (found != m.Headers.end() && found->second == S_GZIP) {
      if (Resources::CompressString(m.Body, compressedBody)) {
        bodyData = compressedBody.data();
        bodyLength = compressedBody.size();
      } else {
        removeContentEncoding = true;
      }
    }
  }

  std::ostringstream headers;
  for (const auto &h : m.Headers) {
    if (h.first == S_ACCEPT_ENCODING || h.first == S_CONTENT_LENGTH ||
        (removeContentEncoding && h.first == S_CONTENT_ENCODING)) {
      continue;
    }
    headers << h.first << ": " << h.second << "\r\n";
  }

  mg_printf(c,
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "%s"
            "Accept-Encoding: gzip\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            m.Method.c_str(), mg_url_uri(m.Url.c_str()), host.c_str(),
            headers.str().c_str(), bodyLength);
  mg_send(c, bodyData, bodyLength);
}

void SendHttpResponse(mg_connection *c, const HttpMessage &m) {
  std::vector<char> compressedBody;
  const char *bodyData = m.Body.data();
  size_t bodyLength = m.Body.size();

  if (!m.Body.empty()) {
    auto found = m.Headers.find(S_CONTENT_ENCODING);
    if (found != m.Headers.end() && found->second == S_GZIP) {
      if (!Resources::CompressString(m.Body, compressedBody)) {
        mg_http_reply(c, 500, NULL, "\n");
        return;
      }
      bodyData = compressedBody.data();
      bodyLength = compressedBody.size();
    }
  }

  std::ostringstream headers;
  for (const auto &h : m.Headers) {
    if (h.first == S_ACCEPT_ENCODING || h.first == S_CONTENT_LENGTH) {
      continue;
    }
    headers << h.first << ": " << h.second << "\r\n";
  }

  mg_printf(c,
            "HTTP/1.1 %d %s\r\n"
            "%s"
            "Content-Length %d\r\n"
            "\r\n",
            m.Code, headers.str().c_str(), bodyLength);
  mg_send(c, bodyData, bodyLength);
}

}  // namespace mongoose
}  // namespace network
}  // namespace core
}  // namespace MGDF
