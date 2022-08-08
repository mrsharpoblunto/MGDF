#include "stdafx.h"

#include "MGDFHttp.hpp"

namespace MGDF {
namespace common {

HttpServer::HttpServer() : _conn(nullptr), _running(false) {
  mg_mgr_init(&_mgr);
}

HttpServer::~HttpServer() {
  _running = false;
  _pollThread.join();
  mg_mgr_free(&_mgr);
}

void HttpServer::Listen(const std::string& port) {
  if (_running) {
    return;
  }
  std::string listenAddress = "0.0.0.0:" + port;
  _conn = mg_http_listen(&_mgr, listenAddress.c_str(),
                         &HttpServer::HandleRequest, this);
  if (_conn) {
    _running = true;
    _pollThread = std::thread([this]() {
      while (_running) {
        mg_mgr_poll(&_mgr, 1000);
      }
    });
  }
}

void HttpServer::HandleRequest(struct mg_connection* c, int ev, void* ev_data,
                               void* fn_data) {
  std::ignore = c;
  std::ignore = ev_data;
  std::ignore = ev;
  HttpServer* server = static_cast<HttpServer*>(fn_data);
  server->OnRequest(c, ev, ev_data);
}

}  // namespace common
}  // namespace MGDF