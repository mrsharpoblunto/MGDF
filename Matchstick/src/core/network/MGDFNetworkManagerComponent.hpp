#pragma once

#include <MGDF/MGDF.h>

#include <memory>
#include <string>

#include "MGDFHttpClient.hpp"
#include "MGDFHttpServer.hpp"
#include "MGDFWebSocket.hpp"

namespace MGDF {
namespace core {
namespace network {

struct NetworkManagerOptions {
  size_t HttpClientOriginConnectionLimit;
  size_t HttpClientConnectionTimeout;
  size_t HttpClientKeepAlive;
  size_t WebSocketClientReconnectInterval;
};

class INetworkManagerComponent {
 public:
  virtual ~INetworkManagerComponent() {}
  virtual std::unique_ptr<IHttpClientRequest> CreateHttpRequest(
      const std::string &url) = 0;
  virtual std::shared_ptr<IHttpServer> CreateHttpServer(
      uint32_t port, const std::string &socketPath) = 0;
  virtual std::shared_ptr<IWebSocket> CreateWebSocket(
      const std::string &url) = 0;
};

}  // namespace network
}  // namespace core
}  // namespace MGDF
