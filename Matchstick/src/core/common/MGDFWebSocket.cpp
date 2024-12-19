#include "stdafx.h"

#include "MGDFWebSocket.hpp"

#include <algorithm>
#include <string>

#include "MGDFCertificates.hpp"
#include "MGDFLoggerImpl.hpp"
#include "MGDFMongooseMemFS.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

constexpr uint64_t RECONNECT_INTERVAL = 3000U;

WebSocketConnection::WebSocketConnection(mg_connection* c,
                                         MGDFWebSocketConnectionState state)
    : _conn(c), _state(state) {}

WebSocketConnection::~WebSocketConnection() {}

void WebSocketConnection::Poll(int ev, void* ev_data, void* fn_data) {
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

MGDFWebSocketConnectionState WebSocketConnection::GetConnectionState(
    std::string& lastError) {
  std::lock_guard<std::mutex> lock(_mutex);
  lastError = _lastError;
  return _state;
}

void WebSocketConnection::Send(const std::vector<char>& data, bool binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto& out = _out.emplace_back(data.size(), binary);
  memcpy_s(out.Data.data(), out.Data.size(), data.data(), data.size());
}

void WebSocketConnection::Send(void* data, size_t dataLength, bool binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto& out = _out.emplace_back(dataLength, binary);
  memcpy_s(out.Data.data(), out.Data.size(), data, dataLength);
}

bool WebSocketConnection::Receive(std::vector<char>& message, bool& binary) {
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

WebSocketClient::WebSocketClient(const std::string& url)
    : WebSocketConnection(nullptr, MGDF_WEBSOCKET_CLOSED),
      _running(true),
      _url(url),
      _disconnected(0),
      _usesTLS(false) {
  MemFS::InitMGFS(_fs);
  MemFS::Ensure(CertificateManager::S_CA_PEM, &CertificateManager::LoadCerts);

  if (url.size() >= 6) {
    std::string lowerUrl = url.substr(0, 6);
    std::transform(lowerUrl.begin(), lowerUrl.end(), lowerUrl.begin(),
                   tolowerChar);
    _usesTLS = lowerUrl == "wss://";
  }

  _pollThread = std::thread([this]() {
    mg_mgr_init(&_mgr);
    while (_running) {
      const auto now = mg_millis();
      if (_conn == nullptr && (now - _disconnected) > RECONNECT_INTERVAL) {
        _conn = mg_ws_connect(&_mgr, _url.c_str(),
                              &WebSocketClient::HandleRequest, this, NULL);
      }
      mg_mgr_poll(&_mgr, 64);
    }
    mg_mgr_free(&_mgr);
  });
}

WebSocketClient::~WebSocketClient() {
  if (_running) {
    _running = false;
    _pollThread.join();
  }
}

void WebSocketClient::HandleRequest(struct mg_connection* c, int ev,
                                    void* ev_data, void* fn_data) {
  std::ignore = c;
  std::ignore = ev_data;
  std::ignore = ev;
  WebSocketClient* client = static_cast<WebSocketClient*>(fn_data);
  switch (ev) {
    case MG_EV_CONNECT:
      if (client->_usesTLS) {
        const auto host = mg_url_host(client->_url.c_str());
        const mg_tls_opts opts = {.ca = CertificateManager::S_CA_PEM.c_str(),
                                  .srvname = host,
                                  .fs = &client->_fs};
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

WebSocketServer::WebSocketServer() : _conn(nullptr), _running(false) {}

WebSocketServer::~WebSocketServer() {
  if (_running) {
    _running = false;
    _pollThread.join();
  }
}

void WebSocketServer::Listen(const std::string& port) {
  if (_running) {
    return;
  }
  std::string listenAddress = "0.0.0.0:" + port;
  mg_mgr_init(&_mgr);
  _conn = mg_http_listen(&_mgr, listenAddress.c_str(),
                         &WebSocketServer::HandleRequest, this);
  if (_conn) {
    _running = true;
    _pollThread = std::thread([this]() {
      while (_running) {
        mg_mgr_poll(&_mgr, 64);
      }
      mg_mgr_free(&_mgr);
    });
  }
}

void WebSocketServer::HandleRequest(struct mg_connection* c, int ev,
                                    void* ev_data, void* fn_data) {
  WebSocketServer* server = static_cast<WebSocketServer*>(fn_data);

  std::shared_ptr<WebSocketConnection> connection;
  {
    std::lock_guard<std::mutex> lock(server->_mutex);
    auto found = server->_connections.find(c->id);
    if (found != server->_connections.end()) {
      connection = found->second;
    }
  }
  if (connection) {
    std::string lastError;
    connection->Poll(ev, ev_data, fn_data);
    if (connection->GetConnectionState(lastError) == MGDF_WEBSOCKET_CLOSED) {
      std::lock_guard<std::mutex> lock(server->_mutex);
      server->_connections.erase(connection->GetId());
    }
  } else {
    switch (ev) {
      case MG_EV_POLL: {
        std::lock_guard<std::mutex> lock(server->_mutex);
        for (auto& conn : server->_connections) {
          conn.second->Poll(ev, ev_data, fn_data);
        }
      } break;
      case MG_EV_OPEN:
        if (!c->is_listening) {
          auto newConnection =
              std::make_shared<WebSocketConnection>(c, MGDF_WEBSOCKET_OPEN);
          {
            std::lock_guard<std::mutex> lock(server->_mutex);
            server->_connections.insert(std::make_pair(c->id, newConnection));
          }
          server->OnConnected(newConnection);
        }
        break;
      case MG_EV_HTTP_MSG: {
        mg_ws_upgrade(c, (struct mg_http_message*)ev_data, NULL);
      } break;
      case MG_EV_ERROR: {
        LOG(static_cast<char*>(ev_data), MGDF_LOG_ERROR);
      } break;
    }
  }
}

}  // namespace core
}  // namespace MGDF
