#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "MGDFResources.hpp"
#include "mongoose.h"

namespace MGDF {
namespace core {

#define mg_stdstr(str) mg_str_n(str.c_str(), str.size())

static const std::string S_CONTENT_ENCODING;
static const std::string S_ACCEPT_ENCODING;
static const std::string S_CONTENT_LENGTH;
static const std::string S_GZIP;

class INetworkEventListener {
 public:
  virtual ~INetworkEventListener() {}
  virtual void OnPoll(mg_mgr &mgr, mg_fs &fs) = 0;
};

class NetworkEventLoop {
 public:
  NetworkEventLoop();
  virtual ~NetworkEventLoop();

  void Add(INetworkEventListener *listener);
  void Remove(INetworkEventListener *listener);

 private:
  struct EventLoopMember {
    mg_mgr Mgr;
    mg_fs Fs;
  };
  std::mutex _mutex;
  std::thread _thread;
  std::unordered_map<INetworkEventListener *, EventLoopMember> _listeners;
  bool _running;
};

struct HttpMessageBase {
  int Code = 0;
  std::string Body;
  std::string Method;
  std::string Error;
  std::unordered_map<std::string, std::string> Headers;
};

template <typename T>
std::shared_ptr<T> CreateHttpMessage(struct mg_http_message *hm) {
  std::shared_ptr<T> response = std::make_shared<T>();

  response->Code = mg_http_status(hm);
  response->Method = std::string(hm->method.ptr, hm->method.len);
  constexpr const size_t max = sizeof(hm->headers) / sizeof(hm->headers[0]);
  for (size_t i = 0; i < max && hm->headers[i].name.len > 0; i++) {
    struct mg_str *k = &hm->headers[i].name, *v = &hm->headers[i].value;
    response->Headers.insert(std::make_pair(std::string(k->ptr, k->len),
                                            std::string(v->ptr, v->len)));
  }

  const auto contentEncodingHeader =
      mg_http_get_header(hm, S_CONTENT_ENCODING.c_str());
  if (contentEncodingHeader) {
    if (mg_strcmp(*contentEncodingHeader, mg_stdstr(S_GZIP)) == 0) {
      Resources::DecompressString(hm->body.ptr, hm->body.len, response->Body);
    } else {
      response->Error = "Unsupported Content-Encoding";
      response->Body.clear();
    }
  } else {
    response->Body.resize(hm->body.len);
    memcpy_s(response->Body.data(), response->Body.size(), hm->body.ptr,
             hm->body.len);
  }
  return response;
}

void SendHttpResponse(struct mg_connection *c, const HttpMessageBase &m);
void SendHttpRequest(struct mg_connection *c, const std::string &host,
                     const std::string &url, HttpMessageBase &m);

}  // namespace core
}  // namespace MGDF
