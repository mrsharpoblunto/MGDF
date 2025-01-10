#pragma once

#include <MGDF/MGDF.h>

#include <functional>
#include <span>
#include <vector>

namespace MGDF {
namespace core {

struct WebSocketMessage {
  WebSocketMessage(size_t size, bool binary) : Data(size), Binary(binary) {}
  std::vector<char> Data;
  bool Binary;
};

class IWebSocket {
 public:
  virtual ~IWebSocket() {}

  virtual IWebSocket *Send(const std::vector<char> &data, bool binary) = 0;
  virtual IWebSocket *Send(void *data, size_t dataLength, bool binary) = 0;
  virtual IWebSocket *OnReceive(
      std::function<void(std::span<const char> &message, bool binary)>
          handler) = 0;
  virtual MGDFWebSocketConnectionState GetConnectionState(
      std::string &lastError) const = 0;
};

}  // namespace core
}  // namespace MGDF
