#pragma once

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace MGDF {
namespace core {
namespace network {

struct HttpMessage {
  int Code = 0;
  std::string Url;
  std::string Body;
  std::string Method;
  std::string Error;
  std::unordered_map<std::string, std::string> Headers;
};

enum class HttpRequestState {
  Requesting,
  Cancelled,
  Error,
  Complete,
};

class IHttpClientPendingRequest {
 public:
  virtual ~IHttpClientPendingRequest() {}
  virtual void CancelRequest() = 0;
  virtual HttpRequestState GetRequestState() const = 0;
  virtual bool GetResponse(std::shared_ptr<HttpMessage> &response) = 0;
};

class IHttpClientRequest {
 public:
  virtual ~IHttpClientRequest() {}
  virtual IHttpClientRequest *SetRequestMethod(const std::string &method) = 0;
  virtual IHttpClientRequest *SetRequestHeader(const std::string &header,
                                               const std::string &value) = 0;
  virtual IHttpClientRequest *SetRequestBody(const char *body,
                                             size_t bodyLength,
                                             bool compress) = 0;
  virtual bool GetRequestHeader(const std::string &header,
                                std::string &value) const = 0;

  virtual std::shared_ptr<IHttpClientPendingRequest> SendRequest(
      std::function<void(std::shared_ptr<HttpMessage> &response)> handler) = 0;
};

}  // namespace network
}  // namespace core
}  // namespace MGDF
