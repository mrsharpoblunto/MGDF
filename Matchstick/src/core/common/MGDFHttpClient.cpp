#include "stdafx.h"

#include "MGDFHttpClient.hpp"

#include <algorithm>
#include <sstream>

#include "MGDFCertificates.hpp"
#include "MGDFVersionInfo.hpp"

#pragma warning(disable : 4706)

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

static const std::string S_CONTENT_TYPE("Content-Type");
static const std::string S_USER_AGENT("User-Agent");
static const std::string S_CONNECTION("Connection");
static const std::string S_KEEP_ALIVE("Keep-Alive");
static const std::string S_ACCEPT("Accept");

static const std::string S_GET("GET");
static const std::string S_HTTP("http://");
static const std::string S_HTTPS("https://");
static const std::string S_APPLICATION_JSON("application/json");
static const std::string S_CLOSE("close");
static const std::string S_TIMEOUT("timeout");
static const std::string S_MAX("max");

std::shared_ptr<HttpClientRequest> HttpClientRequestGroup::GetResponse(
    std::shared_ptr<HttpClientResponse> &response) {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_requests.size()) {
    auto request = _requests.front();
    response = request->_response;
    _requests.pop_front();
    return request;
  }
  return std::shared_ptr<HttpClientRequest>();
}

HttpClientRequest::HttpClientRequest(const std::string &url)
    : _url(url), _state(HttpRequestState::Preparing) {
  static std::ostringstream ua;
  ua << "MGDF/" << MGDFVersionInfo::MGDF_INTERFACE_VERSION
     << " (Windows) Framework/" << MGDFVersionInfo::MGDF_VERSION();
  static std::string uaStr = ua.str();

  _request.Method = S_GET;
  _request.Headers.insert(std::make_pair(S_CONTENT_TYPE, S_APPLICATION_JSON));
  _request.Headers.insert(std::make_pair(S_ACCEPT_ENCODING, S_GZIP));
  _request.Headers.insert(std::make_pair(S_USER_AGENT, uaStr.c_str()));
  _request.Headers.insert(std::make_pair(S_ACCEPT, S_APPLICATION_JSON));
}

HttpClientRequest::~HttpClientRequest() {}

HttpClientRequest *HttpClientRequest::SetBody(const char *body,
                                              size_t bodyLength,
                                              bool compress) {
  std::lock_guard<std::mutex> lock(_mutex);
  _request.Body.assign(body, bodyLength);
  if (compress) {
    _request.Headers.insert(std::make_pair(S_CONTENT_ENCODING, S_GZIP));
  }
  return this;
}

HttpClientRequest *HttpClientRequest::SetHeader(const std::string &header,
                                                const std::string &value) {
  std::lock_guard<std::mutex> lock(_mutex);
  _request.Headers.insert(std::make_pair(header, value));
  return this;
}

HttpClientRequest *HttpClientRequest::SetMethod(const std::string &method) {
  std::lock_guard<std::mutex> lock(_mutex);
  _request.Method = method;
  return this;
}

bool HttpClientRequest::GetHeader(const std::string &header,
                                  std::string &value) const {
  std::lock_guard<std::mutex> lock(_mutex);
  const auto found = _request.Headers.find(header);
  if (found != _request.Headers.end()) {
    value = found->second;
    return true;
  }
  return false;
}

void HttpClientRequest::Cancel() {
  std::unique_lock<std::mutex> lock(_mutex);
  if (_state != HttpRequestState::Error &&
      _state != HttpRequestState::Complete &&
      _state != HttpRequestState::Cancelled) {
    _state = HttpRequestState::Cancelled;
    _response = std::make_shared<HttpClientResponse>();
    _response->Code = -1;
    _response->Error = "Cancelled";
  }
}

bool HttpClientRequest::GetResponse(
    std::shared_ptr<HttpClientResponse> &response) const {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_state == HttpRequestState::Complete ||
      _state == HttpRequestState::Error ||
      _state == HttpRequestState::Cancelled) {
    response = _response;
    return true;
  }
  return false;
}

HttpRequestState HttpClientRequest::GetState() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _state;
}

HttpClientOptions HttpClient::DEFAULT_OPTIONS{
    .OriginConnectionLimit = 4U,
    .ConnectionTimeout = 10000,
    .KeepAlive = 10000,
};

HttpClient::HttpClient(std::shared_ptr<NetworkEventLoop> eventLoop,
                       HttpClientOptions &options)
    : _options(options), _eventLoop(eventLoop) {
  _eventLoop->Add(this);
}

HttpClient::~HttpClient() {
  _eventLoop->Remove(this);

  // clean up any pending requests or requests in progress
  for (const auto &origin : _origins) {
    for (const auto pp : origin.second.PendingRequests) {
      const auto &p = pp.first;
      std::unique_lock<std::mutex> lock(p->_mutex);
      p->_state = HttpRequestState::Cancelled;
      p->_response = std::make_shared<HttpClientResponse>();
      p->_response->Code = -1;
      p->_response->Error = "Cancelled";
      if (pp.second) {
        std::lock_guard<std::mutex> gLock(pp.second->_mutex);
        pp.second->_requests.push_back(p);
      }
    }
    for (auto &c : origin.second.Connections) {
      if (c.second.Request) {
        const auto &p = c.second.Request;
        std::unique_lock<std::mutex> lock(p->_mutex);
        p->_state = HttpRequestState::Cancelled;
        p->_response = std::make_shared<HttpClientResponse>();
        p->_response->Code = -1;
        p->_response->Error = "Cancelled";
        if (c.second.Group) {
          const auto &g = c.second.Group;
          std::lock_guard<std::mutex> gLock(g->_mutex);
          g->_requests.push_back(p);
        }
      }
    }
  }
}

void HttpClient::SendRequest(std::shared_ptr<HttpClientRequest> request,
                             std::shared_ptr<HttpClientRequestGroup> group) {
  {
    std::lock_guard<std::mutex> rLock(request->_mutex);
    if (request->_state == HttpRequestState::Preparing) {
      request->_state = HttpRequestState::Pending;
      std::lock_guard<std::mutex> lock(_mutex);
      _pendingRequests.push_back(std::make_pair(request, group));
    }
  }
}

void HttpClient::OnPoll(mg_mgr &mgr, mg_fs &fs) {
  // any pending requests that have not been assigned to a connection
  // will require a new connection to be created. If we've maxxed out the
  // allowable connections to this domain, then the requests will stay
  // pending until some requests complete
  for (auto &origin : _origins) {
    auto connections = origin.second.Connections.size();
    while (origin.second.PendingRequests.size() &&
           connections < origin.second.ConnectionLimit) {
      auto p = origin.second.PendingRequests.front();
      origin.second.PendingRequests.pop_front();
      HttpConnection *newConnection = new HttpConnection{
          .Request = p.first,
          .Group = p.second,
          .KeepAliveDuration = _options.KeepAlive,
          .ConnectTimeoutDuration = _options.ConnectionTimeout,
          .Origin = &origin.second,
      };
      ++connections;
      mg_http_connect(&mgr, p.first->_url.c_str(), &HttpClient::HandleResponse,
                      newConnection);
    }
  }

  std::vector<std::pair<std::shared_ptr<HttpClientRequest>,
                        std::shared_ptr<HttpClientRequestGroup>>>
      pending;
  {
    std::lock_guard<std::mutex> lock(_mutex);
    pending.clear();
    pending.assign(_pendingRequests.begin(), _pendingRequests.end());
    _pendingRequests.clear();
  }

  // assign all valid pending requests to an origin
  for (const auto &pp : pending) {
    const auto &p = pp.first;
    std::unique_lock<std::mutex> pLock(p->_mutex);

    std::string url = p->_url;
    std::transform(url.begin(), url.end(), url.begin(), tolowerChar);

    if (!url.starts_with(S_HTTP) && !url.starts_with(S_HTTPS)) {
      p->_state = HttpRequestState::Error;
      p->_response = std::make_shared<HttpClientResponse>();
      p->_response->Error = "Invalid URL protocol";
      if (pp.second) {
        std::lock_guard<std::mutex> gLock(pp.second->_mutex);
        pp.second->_requests.push_back(p);
      }
      continue;
    }

    const struct mg_str host = mg_url_host(url.c_str());
    const unsigned short port = mg_url_port(url.c_str());
    const bool usesTLS = mg_url_is_ssl(url.c_str());
    // uri might be case sensitive or have unicode characters, so use the
    // original url
    const char *uri = mg_url_uri(p->_url.c_str());

    // to tell origins http/https origins using default ports
    // apart we need to include the port in all cases
    std::ostringstream canonicalHost;
    std::ostringstream canonicalHostAndPort;
    canonicalHost << std::string(host.ptr, host.len);
    canonicalHostAndPort << std::string(host.ptr, host.len) << ":" << port;
    if ((usesTLS && port != 443) || !(usesTLS && port != 80)) {
      canonicalHost << ":" << port;
    }

    std::ostringstream canonicalURL;
    canonicalURL << (usesTLS ? S_HTTPS : S_HTTP) << canonicalHost.str()
                 << (uri[0] == '\\' ? "/" : uri);

    auto origin = _origins.find(canonicalHostAndPort.str());
    if (origin == _origins.end()) {
      origin = _origins
                   .emplace(canonicalHostAndPort.str(),
                            HttpOrigin{.UsesTLS = usesTLS,
                                       .ConnectionLimit =
                                           _options.OriginConnectionLimit,
                                       .Host = canonicalHost.str(),
                                       .MemFS = &fs})
                   .first;
    }

    p->_url = canonicalURL.str();
    origin->second.PendingRequests.push_back(pp);
  }
}

void HttpClient::HandleResponse(struct mg_connection *c, int ev, void *ev_data,
                                void *fn_data) {
  HttpConnection *conn = static_cast<HttpConnection *>(fn_data);
  if (ev == MG_EV_OPEN) {
    conn->ConnectTimeout = mg_millis() + conn->ConnectTimeoutDuration;
    conn->KeepAlive = mg_millis() + conn->KeepAliveDuration;
    conn->Connection = c;
    const auto &added =
        conn->Origin->Connections.insert(std::make_pair(c, *conn));
    c->fn_data = &added.first->second;
    // now that we've added this to the mapped list of connections, we need
    // to free the memory from the HttpConnection's dynamic allocation above
    delete conn;
  } else if (ev == MG_EV_POLL) {
    if (c->is_closing || c->is_draining) {
      return;
    }

    if (conn->Request) {
      // if we have an active request associated with this connection
      std::unique_lock<std::mutex> lock(conn->Request->_mutex);
      // check for connection timeout
      if (mg_millis() > conn->ConnectTimeout &&
          (c->is_connecting || c->is_resolving)) {
        lock.unlock();
        // close the connection
        mg_error(c, "Connect timeout");
      }
    } else {
      // this connection is idle
      // timeout any idle keepalive connections
      if (mg_millis() > conn->KeepAlive) {
        c->is_draining = 1;
      } else if (!conn->Origin->PendingRequests.empty()) {
        // if we have some pending requests, then we can reuse this
        // connection
        auto request = conn->Origin->PendingRequests.front();
        conn->Origin->PendingRequests.pop_front();
        conn->Origin->IdleConnections.erase(c);
        conn->Request = request.first;
        conn->Group = request.second;
        if (!MakeRequestWithConnection(conn)) {
          conn->Request.reset();
          conn->Group.reset();
          conn->Origin->IdleConnections.insert(std::make_pair(c, conn));
        }
      }
    }
  } else if (ev == MG_EV_CONNECT) {
    if (conn->Origin->UsesTLS) {
      const mg_tls_opts opts = {.ca = CertificateManager::S_CA_PEM.c_str(),
                                .srvname = mg_stdstr(conn->Origin->Host),
                                .fs = conn->Origin->MemFS};
      mg_tls_init(c, &opts);
    }
    if (!MakeRequestWithConnection(conn)) {
      conn->Request.reset();
      conn->Group.reset();
      if (conn->KeepAliveDuration > 0) {
        // if keep alives are enabled, keep the connection around for re-use
        conn->Origin->IdleConnections.insert(std::make_pair(c, conn));
      } else {
        c->is_closing = 1;
      }
    }
  } else if (ev == MG_EV_HTTP_MSG) {
    // Connected to server
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    GetResponseFromConnection(conn, hm);
    conn->Request.reset();
    conn->Group.reset();
    if (conn->KeepAliveDuration > 0) {
      // if keep alives are enabled, keep the connection around for re-use
      conn->Origin->IdleConnections.insert(std::make_pair(c, conn));
    } else {
      c->is_closing = 1;
    }
  } else if (ev == MG_EV_ERROR) {
    if (conn->Request) {
      std::unique_lock<std::mutex> lock(conn->Request->_mutex);
      conn->Request->_state = HttpRequestState::Error;
      conn->Request->_response = std::make_shared<HttpClientResponse>();
      conn->Request->_response->Error =
          std::string(static_cast<char *>(ev_data));
      if (conn->Group) {
        std::lock_guard<std::mutex> gLock(conn->Group->_mutex);
        conn->Group->_requests.push_back(conn->Request);
      }
      conn->Request.reset();
      conn->Group.reset();
    }
  } else if (ev == MG_EV_CLOSE) {
    conn->Origin->IdleConnections.erase(c);
    conn->Origin->Connections.erase(c);
  }
}

bool HttpClient::MakeRequestWithConnection(HttpConnection *conn) {
  std::lock_guard<std::mutex> lock(conn->Request->_mutex);
  auto request = conn->Request;

  if (request->_state == HttpRequestState::Cancelled) {
    return false;
  }
  request->_state = HttpRequestState::Requesting;
  SendHttpRequest(conn->Connection, conn->Origin->Host, request->_url,
                  request->_request);

  if (request->_request.Error.size()) {
    request->_state = HttpRequestState::Error;
    if (conn->Group) {
      std::lock_guard<std::mutex> gLock(conn->Group->_mutex);
      conn->Group->_requests.push_back(request);
    }
    return false;
  } else {
    conn->KeepAlive = mg_millis() + conn->KeepAliveDuration;
    return true;
  }
}

void HttpClient::GetResponseFromConnection(HttpConnection *conn,
                                           mg_http_message *hm) {
  std::lock_guard<std::mutex> lock(conn->Request->_mutex);
  const auto request = conn->Request;
  if (request->_state == HttpRequestState::Cancelled) {
    return;
  }

  const auto connectionHeader = mg_http_get_header(hm, S_CONNECTION.c_str());
  if (connectionHeader &&
      mg_strcmp(*connectionHeader, mg_stdstr(S_CLOSE)) == 0) {
    conn->KeepAliveDuration = 0;
    conn->KeepAlive = mg_millis();
  }

  if (conn->KeepAliveDuration > 0) {
    const auto keepAliveHeader = mg_http_get_header(hm, S_KEEP_ALIVE.c_str());
    if (keepAliveHeader) {
      int timeout = -1, maxRequests = -1;
      ParseKeepAliveHeader(keepAliveHeader, timeout, maxRequests);
      if (timeout > 0) {
        conn->KeepAliveDuration = timeout;
      } else if (maxRequests == 0) {
        conn->KeepAliveDuration = 0;
      }
    }
  }
  conn->KeepAlive = mg_millis() + conn->KeepAliveDuration;

  request->_response = CreateHttpMessage<HttpClientResponse>(hm);
  request->_state = request->_response->Error.size()
                        ? HttpRequestState::Error
                        : HttpRequestState::Complete;
  if (conn->Group) {
    std::lock_guard<std::mutex> gLock(conn->Group->_mutex);
    conn->Group->_requests.push_back(request);
  }
}

void HttpClient::ParseKeepAliveHeader(const mg_str *header, int &timeout,
                                      int &max) {
  if (header == nullptr || header->len == 0) {
    return;
  }

  const char *start = header->ptr;
  const char *end = start + header->len;

  while (start < end) {
    // Skip leading whitespace
    while (start < end && std::isspace(*start)) ++start;

    // Check if it's "timeout" or "max"
    if (start + 7 <= end &&
        mg_strcmp(mg_str_n(start, 7), mg_stdstr(S_TIMEOUT)) == 0) {
      start += 7;
      if (*start == '=') {
        ++start;
        timeout = 0;
        while (start < end && std::isdigit(*start)) {
          timeout = timeout * 10 + (*start - '0');
          ++start;
        }
      }
    } else if (start + 3 <= end &&
               mg_strcmp(mg_str_n(start, 3), mg_stdstr(S_MAX)) == 0) {
      start += 3;
      if (*start == '=') {
        ++start;
        max = 0;
        while (start < end && std::isdigit(*start)) {
          max = max * 10 + (*start - '0');
          ++start;
        }
      }
    }

    // Move to next directive
    while (start < end && *start != ',') ++start;
    if (start < end && *start == ',') ++start;
  }
}

}  // namespace core
}  // namespace MGDF