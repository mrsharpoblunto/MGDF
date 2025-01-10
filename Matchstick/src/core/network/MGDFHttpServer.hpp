#pragma once

#include <functional>
#include <memory>
#include <string>

#include "MGDFWebSocket.hpp"

namespace MGDF {
namespace core {
namespace network {

class IHttpServerRequest {
 public:
  virtual ~IHttpServerRequest() {}

  virtual bool GetRequestHeader(const std::string &name,
                                std::string &value) const = 0;
  virtual bool HasRequestHeader(const std::string &name) const = 0;
  virtual const std::string &GetRequestHeader(
      const std::string &name) const = 0;
  virtual const std::string &GetRequestMethod() const = 0;
  virtual const std::string &GetRequestUrl() const = 0;
  virtual const std::string &GetRequestBody() const = 0;

  virtual IHttpServerRequest *SetResponseCode(int code) = 0;
  virtual IHttpServerRequest *SetResponseHeader(const std::string &name,
                                                const std::string &value) = 0;
  virtual IHttpServerRequest *SetResponseBody(const char *body,
                                              size_t bodyLength) = 0;
  virtual void SendResponse() = 0;
};

class IHttpServer {
 public:
  virtual ~IHttpServer() {}

  virtual IHttpServer *OnHttpRequest(
      std::function<void(std::shared_ptr<IHttpServerRequest> request)>
          handler) = 0;
  virtual IHttpServer *OnWebSocketRequest(
      std::function<void(std::shared_ptr<IWebSocket> socket)> handler) = 0;
};

}  // namespace network
}  // namespace core
}  // namespace MGDF
