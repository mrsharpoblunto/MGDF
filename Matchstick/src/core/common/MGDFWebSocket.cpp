#include "stdafx.h"

#include "MGDFWebSocket.hpp"

#include <algorithm>
#include <string>

#include "MGDFCertificates.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

constexpr uint64_t RECONNECT_INTERVAL = 3000U;

WebSocketConnectionBase::WebSocketConnectionBase(
    mg_connection* c, MGDFWebSocketConnectionState state)
    : _conn(c), _state(state) {}

WebSocketConnectionBase::~WebSocketConnectionBase() {}

void WebSocketConnectionBase::Poll(int ev, void* ev_data, void* fn_data) {
  std::ignore = ev;
  std::ignore = fn_data;

  switch (ev) {
    case MG_EV_POLL: {
      // send outgoing requests
      std::unique_lock<std::mutex> lock(_mutex);
      if (!_out.size()) {
        return;
      }
      std::vector<WebSocketMessage> out = std::move(_out);
      _out.clear();
      lock.unlock();
      for (const auto& o : out) {
        mg_ws_send(_conn, o.Data.data(), o.Data.size(),
                   o.Binary ? WEBSOCKET_OP_BINARY : WEBSOCKET_OP_TEXT);
      }
    } break;
    case MG_EV_OPEN: {
      std::lock_guard<std::mutex> lock(_mutex);
      _state = MGDF_WEBSOCKET_OPEN;
      _lastError.clear();
    } break;
    case MG_EV_HTTP_MSG: {
      mg_ws_upgrade(_conn, (struct mg_http_message*)ev_data, NULL);
    } break;
    case MG_EV_WS_MSG: {
      struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
      std::lock_guard<std::mutex> lock(_mutex);
      const auto op = wm->flags & 15;
      auto& in = _in.emplace_back(
          std::piecewise_construct, std::forward_as_tuple(wm->data.len),
          std::forward_as_tuple(op == WEBSOCKET_OP_BINARY));
      memcpy_s(in.first.data(), in.first.size(), wm->data.ptr, wm->data.len);
    } break;
    case MG_EV_ERROR: {
      std::lock_guard<std::mutex> lock(_mutex);
      _lastError = std::string((const char*)ev_data);
    } break;
    case MG_EV_CLOSE: {
      std::lock_guard<std::mutex> lock(_mutex);
      _state = MGDF_WEBSOCKET_CLOSED;
    } break;
  }
}

MGDFWebSocketConnectionState WebSocketConnectionBase::GetConnectionState(
    std::string& lastError) {
  std::lock_guard<std::mutex> lock(_mutex);
  lastError = _lastError;
  return _state;
}

void WebSocketConnectionBase::Send(const std::vector<char>& data, bool binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto& out = _out.emplace_back(data.size(), binary);
  memcpy_s(out.Data.data(), out.Data.size(), data.data(), data.size());
}

void WebSocketConnectionBase::Send(void* data, size_t dataLength, bool binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto& out = _out.emplace_back(dataLength, binary);
  memcpy_s(out.Data.data(), out.Data.size(), data, dataLength);
}

bool WebSocketConnectionBase::Receive(std::vector<char>& message,
                                      bool& binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_in.size()) {
    auto& in = _in.front();
    message = std::move(in.first);
    binary = in.second;
    _in.pop_front();
    return true;
  } else {
    return false;
  }
}

WebSocketClientConnection::WebSocketClientConnection(
    std::shared_ptr<NetworkEventLoop>& eventLoop, const std::string& url)
    : WebSocketConnectionBase(nullptr, MGDF_WEBSOCKET_CLOSED),
      _eventLoop(eventLoop),
      _url(url),
      _disconnected(0),
      _usesTLS(false) {
  _eventLoop->Add(this);
  if (url.size() >= 6) {
    std::string lowerUrl = url.substr(0, 6);
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(),
                   tolowerChar);
    _usesTLS = lowerUrl == "wss://";
  }
}

void WebSocketClientConnection::OnPoll(mg_mgr& mgr, mg_fs& fs) {
  _fs = &fs;
  const auto now = mg_millis();
  if (_conn == nullptr && (now - _disconnected) > RECONNECT_INTERVAL) {
    _conn =
        mg_ws_connect(&mgr, _url.c_str(),
                      &WebSocketClientConnection::HandleRequest, this, NULL);
  }
}

WebSocketClientConnection::~WebSocketClientConnection() {
  _eventLoop->Remove(this);
}

void WebSocketClientConnection::HandleRequest(struct mg_connection* c, int ev,
                                              void* ev_data, void* fn_data) {
  std::ignore = c;
  std::ignore = ev_data;
  std::ignore = ev;
  WebSocketClientConnection* client =
      static_cast<WebSocketClientConnection*>(fn_data);
  switch (ev) {
    case MG_EV_CONNECT:
      if (client->_usesTLS) {
        const auto host = mg_url_host(client->_url.c_str());
        const mg_tls_opts opts = {.ca = CertificateManager::S_CA_PEM.c_str(),
                                  .srvname = host,
                                  .fs = client->_fs};
        mg_tls_init(c, &opts);
      }
      break;
    case MG_EV_CLOSE: {
      client->_conn = nullptr;
      client->_disconnected = mg_millis();
    } break;
  }
  client->Poll(ev, ev_data, fn_data);
}

}  // namespace core
}  // namespace MGDF
