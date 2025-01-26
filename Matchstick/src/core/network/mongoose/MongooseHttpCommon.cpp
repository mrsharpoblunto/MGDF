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

static const char *GetHttpStatusString(uint32_t statusCode) {
  switch (statusCode) {
    case 100:
      return "Continue";
    case 201:
      return "Created";
    case 202:
      return "Accepted";
    case 204:
      return "No Content";
    case 206:
      return "Partial Content";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 304:
      return "Not Modified";
    case 400:
      return "Bad Request";
    case 401:
      return "Unauthorized";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 418:
      return "I'm a teapot";
    case 500:
      return "Internal Server Error";
    case 501:
      return "Not Implemented";
    default:
      return "OK";
  }
}

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
      response.Headers.find(S_CONTENT_ENCODING.c_str());
  if (contentEncodingHeader != response.Headers.end()) {
    if (contentEncodingHeader->second == S_GZIP) {
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
  bool stripContentEncoding = false;

  if (!m.Body.empty()) {
    auto found = m.Headers.find(S_CONTENT_ENCODING);
    if (found != m.Headers.end() && found->second == S_GZIP) {
      if (Resources::CompressString(m.Body, compressedBody)) {
        bodyData = compressedBody.data();
        bodyLength = compressedBody.size();
      } else {
        stripContentEncoding = true;
      }
    }
  }

  std::ostringstream headers;
  for (const auto &h : m.Headers) {
    if (h.first == S_ACCEPT_ENCODING || h.first == S_CONTENT_LENGTH ||
        (h.first == S_CONTENT_ENCODING && stripContentEncoding)) {
      continue;
    }
    headers << h.first << ": " << h.second << "\r\n";
  }

  mg_printf(c,
            "HTTP/1.1 %d %s\r\n"
            "%s"
            "Content-Length %d\r\n"
            "\r\n",
            m.Code, GetHttpStatusString(m.Code), headers.str().c_str(),
            bodyLength);
  mg_send(c, bodyData, bodyLength);
  c->is_resp = 0;
}

}  // namespace mongoose
}  // namespace network
}  // namespace core
}  // namespace MGDF
