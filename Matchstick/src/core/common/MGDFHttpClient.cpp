#include "stdafx.h"

#include "MGDFHttpClient.hpp"

#include <sstream>

namespace MGDF {
namespace core {

HttpClient::HttpClient() : _running(false) {}
HttpClient::~HttpClient() {}

int HttpClient::PostJson(const std::string& url, const Json::Value& content,
                         size_t timeout) {
  _running = true;
  _timeout = timeout;
  _url = url;
  _content = &content;
  _responseCode = 0;
  _lastError.clear();

  mg_mgr_init(&_mgr);
  mg_http_connect(&_mgr, url.c_str(), &HttpClient::HandleResponse, this);
  while (_running) mg_mgr_poll(&_mgr, 50);
  mg_mgr_free(&_mgr);

  return _responseCode;
}

void HttpClient::HandleResponse(struct mg_connection* c, int ev, void* ev_data,
                                void* fn_data) {
  HttpClient* client = static_cast<HttpClient*>(fn_data);
  if (ev == MG_EV_OPEN) {
    // Connection created. Store connect expiration time in c->label
    *(uint64_t*)c->label = mg_millis() + client->_timeout;
  } else if (ev == MG_EV_POLL) {
    if (mg_millis() > *(uint64_t*)c->label &&
        (c->is_connecting || c->is_resolving)) {
      mg_error(c, "Connect timeout");
    }
  } else if (ev == MG_EV_CONNECT) {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(client->_url.c_str());

    if (mg_url_is_ssl(client->_url.c_str())) {
      struct mg_tls_opts opts = {.ca = nullptr, .srvname = host};
      mg_tls_init(c, &opts);
    }

    std::ostringstream oss;
    oss << *client->_content;
    std::string postData = oss.str();

    // Send request
    mg_printf(c,
              "%s %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-Type: application/json\r\n"
              "Content-Length: %d\r\n"
              "\r\n",
              "POST", mg_url_uri(client->_url.c_str()), (int)host.len, host.ptr,
              postData.size());
    mg_send(c, postData.data(), postData.size());
  } else if (ev == MG_EV_HTTP_MSG) {
    // Response is received. Print it
    struct mg_http_message* hm = (struct mg_http_message*)ev_data;
    client->_responseCode = mg_http_status(hm);
    std::string response(hm->message.ptr, (int)hm->message.len);
    c->is_closing = 1;         // Tell mongoose to close this connection
    client->_running = false;  // Tell event loop to stop
  } else if (ev == MG_EV_ERROR) {
    client->_lastError = std::string(static_cast<char*>(ev_data));
    client->_running = false;  // Tell event loop to stop
  }
}

}  // namespace core
}  // namespace MGDF