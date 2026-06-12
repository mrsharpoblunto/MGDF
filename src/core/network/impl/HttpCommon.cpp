#include "stdafx.h"

#include "HttpCommon.hpp"

#include <cctype>

#include "../../common/MGDFResources.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

static const std::string S_CONTENT_ENCODING("Content-Encoding");
static const std::string S_GZIP("gzip");

namespace MGDF {
namespace core {
namespace network {
namespace impl {

const char *GetHttpStatusString(int statusCode) {
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

void EncodeBody(const HttpMessage &m, std::vector<char> &storage,
                const char *&data, size_t &length,
                bool &stripContentEncoding) {
  data = m.Body.data();
  length = m.Body.size();
  stripContentEncoding = false;

  if (!m.Body.empty()) {
    const auto found = m.Headers.find(S_CONTENT_ENCODING);
    if (found != m.Headers.end() && found->second == S_GZIP) {
      if (Resources::CompressString(m.Body, storage)) {
        data = storage.data();
        length = storage.size();
      } else {
        stripContentEncoding = true;
      }
    }
  }
}

void DecodeBody(const char *body, size_t bodyLength, HttpMessage &m) {
  if (!body || !bodyLength) {
    m.Body.clear();
    return;
  }
  const auto contentEncodingHeader = m.Headers.find(S_CONTENT_ENCODING);
  if (contentEncodingHeader != m.Headers.end()) {
    if (contentEncodingHeader->second == S_GZIP) {
      Resources::DecompressString(body, bodyLength, m.Body);
    } else {
      m.Error = "Unsupported Content-Encoding";
      m.Body.clear();
    }
  } else {
    m.Body.assign(body, bodyLength);
  }
}

bool CaseInsensitiveStartsWith(const std::string &str,
                               const std::string &prefix) {
  if (str.size() < prefix.size()) {
    return false;
  }
  for (size_t i = 0; i < prefix.size(); ++i) {
    if (std::tolower(static_cast<unsigned char>(str[i])) !=
        std::tolower(static_cast<unsigned char>(prefix[i]))) {
      return false;
    }
  }
  return true;
}

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
