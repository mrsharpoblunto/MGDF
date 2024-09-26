#include "stdafx.h"

#include "MGDFWebSocket.hpp"

#include <algorithm>
#include <string>

#include "MGDFCertificates.hpp"
#include "MGDFMongooseMemFS.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

constexpr uint64_t RECONNECT_INTERVAL = 3000U;

WebSocketClient::WebSocketClient(const std::string& url)
    : _conn(nullptr),
      _running(true),
      _url(url),
      _disconnected(0U),
      _state(MGDF_WEBSOCKET_CLOSED),
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
    mg_log_set(MG_LL_DEBUG);
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
  if (ev == MG_EV_POLL) {
    // send outgoing requests
    std::unique_lock<std::mutex> lock(client->_mutex);
    if (!client->_out.size()) {
      return;
    }
    std::vector<WebSocketMessage> out = std::move(client->_out);
    client->_out.clear();
    lock.unlock();
    for (const auto& o : out) {
      mg_ws_send(c, o.Data.data(), o.Data.size(),
                 o.Binary ? WEBSOCKET_OP_BINARY : WEBSOCKET_OP_TEXT);
    }
  } else if (ev == MG_EV_CONNECT) {
    if (client->_usesTLS) {
      const auto host = mg_url_host(client->_url.c_str());
      const mg_tls_opts opts = {.ca = CertificateManager::S_CA_PEM.c_str(),
                                .srvname = host,
                                .fs = &client->_fs};
      mg_tls_init(c, &opts);
    }
  } else if (ev == MG_EV_OPEN) {
    std::lock_guard<std::mutex> lock(client->_mutex);
    client->_state = MGDF_WEBSOCKET_OPEN;
    client->_lastError.clear();
  } else if (ev == MG_EV_HTTP_MSG) {
    mg_ws_upgrade(c, (struct mg_http_message*)ev_data, NULL);
  } else if (ev == MG_EV_WS_MSG) {
    struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
    std::lock_guard<std::mutex> lock(client->_mutex);
    auto& in = client->_in.emplace_back(wm->data.len);
    memcpy_s(in.data(), in.size(), wm->data.ptr, wm->data.len);
  } else if (ev == MG_EV_ERROR) {
    std::lock_guard<std::mutex> lock(client->_mutex);
    client->_lastError = std::string((const char*)ev_data);
  } else if (ev == MG_EV_CLOSE) {
    client->_conn = nullptr;
    client->_disconnected = mg_millis();
    std::lock_guard<std::mutex> lock(client->_mutex);
    client->_state = MGDF_WEBSOCKET_CLOSED;
  }
}

MGDFWebSocketConnectionState WebSocketClient::GetConnectionState(
    std::string& lastError) {
  std::lock_guard<std::mutex> lock(_mutex);
  lastError = _lastError;
  return _state;
}

void WebSocketClient::Send(const std::vector<char>& data, bool binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto& out = _out.emplace_back(data.size(), binary);
  memcpy_s(out.Data.data(), out.Data.size(), data.data(), data.size());
}

void WebSocketClient::Send(void* data, size_t dataLength, bool binary) {
  std::lock_guard<std::mutex> lock(_mutex);
  auto& out = _out.emplace_back(dataLength, binary);
  memcpy_s(out.Data.data(), out.Data.size(), data, dataLength);
}

bool WebSocketClient::Receive(std::vector<char>& message) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_in.size()) {
    auto& in = _in.front();
    message = std::move(in);
    _in.pop_front();
    return true;
  } else {
    return false;
  }
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
        std::lock_guard<std::mutex> lock(_mutex);
        mg_mgr_poll(&_mgr, 64);
      }
      mg_mgr_free(&_mgr);
    });
  }
}

void WebSocketServer::Send(const std::vector<char>& data, bool binary) {
  if (_conn) {
    std::lock_guard<std::mutex> lock(_mutex);
    mg_ws_send(_conn, data.data(), data.size(),
               binary ? WEBSOCKET_OP_TEXT : WEBSOCKET_OP_BINARY);
  }
}

void WebSocketServer::HandleRequest(struct mg_connection* c, int ev,
                                    void* ev_data, void* fn_data) {
  std::ignore = c;
  std::ignore = ev_data;
  std::ignore = ev;
  if (ev == MG_EV_HTTP_MSG) {
    mg_ws_upgrade(c, (struct mg_http_message*)ev_data, NULL);
  } else if (ev == MG_EV_WS_MSG) {
    WebSocketServer* server = static_cast<WebSocketServer*>(fn_data);
    server->OnRequest(c, (struct mg_ws_message*)ev_data);
  }
}

}  // namespace core
}  // namespace MGDF
