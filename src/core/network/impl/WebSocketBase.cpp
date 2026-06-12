#include "stdafx.h"

#include "WebSocketBase.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace network {
namespace impl {

IWebSocket *WebSocketBase::Send(const std::vector<char> &data, bool binary) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  auto &buffer = _out.emplace_back(data.size(), binary);
  memcpy_s(buffer.Data.data(), buffer.Data.size(), data.data(), data.size());
  return this;
}

IWebSocket *WebSocketBase::Send(void *data, size_t dataLength, bool binary) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  auto &buffer = _out.emplace_back(dataLength, binary);
  memcpy_s(buffer.Data.data(), buffer.Data.size(), data, dataLength);
  return this;
}

IWebSocket *WebSocketBase::OnReceive(
    std::function<void(std::span<const char> &message, bool binary)> handler) {
  std::unique_lock<std::shared_mutex> lock(_mutex);
  _inHandler = handler;
  return this;
}

MGDFWebSocketConnectionState WebSocketBase::GetConnectionState(
    std::string &lastError) const {
  std::shared_lock<std::shared_mutex> lock(_mutex);
  lastError = _lastError;
  return _state;
}

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
