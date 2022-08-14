#include "stdafx.h"

#include "MGDFHttp.hpp"

namespace MGDF {
namespace common {

HttpServer::HttpServer() : _conn(nullptr), _running(false) {}

HttpServer::~HttpServer() {
  _running = false;
  _pollThread.join();
}

void HttpServer::Listen(const std::string& port) {
  if (_running) {
    return;
  }
  std::string listenAddress = "0.0.0.0:" + port;
  mg_mgr_init(&_mgr);
  _conn = mg_http_listen(&_mgr, listenAddress.c_str(),
                         &HttpServer::HandleRequest, this);
  if (_conn) {
    _running = true;
    _pollThread = std::thread([this]() {
      while (_running) {
        mg_mgr_poll(&_mgr, 1000);
      }
      mg_mgr_free(&_mgr);
    });
  }
}

void HttpServer::HandleRequest(struct mg_connection* c, int ev, void* ev_data,
                               void* fn_data) {
  std::ignore = c;
  std::ignore = ev_data;
  std::ignore = ev;
  if (ev == MG_EV_HTTP_MSG || ev == MG_EV_HTTP_CHUNK) {
    HttpServer* server = static_cast<HttpServer*>(fn_data);
    struct mg_http_message* hm = (struct mg_http_message*)ev_data;
    server->OnRequest(c, hm);
  }
}

}  // namespace common
}  // namespace MGDF