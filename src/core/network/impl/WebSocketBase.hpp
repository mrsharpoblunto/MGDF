#pragma once

#include <functional>
#include <shared_mutex>
#include <string>
#include <vector>

#include "../MGDFWebSocket.hpp"

namespace MGDF {
namespace core {
namespace network {
namespace impl {

class WebSocketBase : public IWebSocket {
 public:
  WebSocketBase(MGDFWebSocketConnectionState state) : _state(state) {}
  virtual ~WebSocketBase() {}
  IWebSocket *Send(const std::vector<char> &data, bool binary) override;
  IWebSocket *Send(void *data, size_t dataLength, bool binary) override;
  IWebSocket *OnReceive(
      std::function<void(std::span<const char> &message, bool binary)> handler)
      final;
  MGDFWebSocketConnectionState GetConnectionState(
      std::string &lastError) const final;

 protected:
  std::function<void(std::span<const char> &message, bool binary)> _inHandler;
  std::vector<WebSocketMessage> _out;
  std::string _lastError;
  MGDFWebSocketConnectionState _state;
  mutable std::shared_mutex _mutex;
};

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
