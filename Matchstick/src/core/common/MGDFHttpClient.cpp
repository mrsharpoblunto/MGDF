#include "stdafx.h"

#include "MGDFHttpClient.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <algorithm>
#include <sstream>
#include <string_view>

#include "MGDFResources.hpp"
#include "MGDFVersionInfo.hpp"

#pragma warning(disable : 4706)

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

static const std::string S_CONTENT_ENCODING("Content-Encoding");
static const std::string S_ACCEPT_ENCODING("Accept-Encoding");
static const std::string S_CONTENT_LENGTH("Content-Length");
static const std::string S_CONTENT_TYPE("Content-Type");
static const std::string S_USER_AGENT("User-Agent");
static const std::string S_CONNECTION("Connection");
static const std::string S_KEEP_ALIVE("Keep-Alive");
static const std::string S_ACCEPT("Accept");

static const std::string S_GET("GET");
static const std::string S_GZIP("gzip");
static const std::string S_HTTP("http://");
static const std::string S_HTTPS("https://");
static const std::string S_APPLICATION_JSON("application/json");
static const std::string S_CLOSE("close");
static const std::string S_TIMEOUT("timeout");
static const std::string S_MAX("max");
static const std::string S_CA_PEM("ca.pem");

#define mg_stdstr(str) mg_str_n(str.c_str(), str.size())

HttpRequest::HttpRequest(const std::string &url)
    : _url(url), _method(S_GET), _state(HttpRequestState::Preparing) {
  static std::ostringstream ua;
  ua << "MGDF/" << MGDFVersionInfo::MGDF_INTERFACE_VERSION
     << " (Windows) Framework/" << MGDFVersionInfo::MGDF_VERSION();
  static std::string uaStr = ua.str();

  _headers.insert(std::make_pair(S_CONTENT_TYPE, S_APPLICATION_JSON));
  _headers.insert(std::make_pair(S_ACCEPT_ENCODING, S_GZIP));
  _headers.insert(std::make_pair(S_USER_AGENT, uaStr.c_str()));
  _headers.insert(std::make_pair(S_ACCEPT, S_APPLICATION_JSON));
}

HttpRequest::~HttpRequest() {}

HttpRequest *HttpRequest::SetBody(const char *body, size_t bodyLength,
                                  bool compress) {
  std::lock_guard<std::mutex> lock(_mutex);
  _body.assign(body, bodyLength);
  if (compress) {
    _headers.insert(std::make_pair(S_CONTENT_ENCODING, S_GZIP));
  }
  return this;
}

HttpRequest *HttpRequest::SetHeader(const std::string &header,
                                    const std::string &value) {
  std::lock_guard<std::mutex> lock(_mutex);
  _headers.insert(std::make_pair(header, value));
  return this;
}

HttpRequest *HttpRequest::SetMethod(const std::string &method) {
  std::lock_guard<std::mutex> lock(_mutex);
  _method = method;
  return this;
}

bool HttpRequest::GetHeader(const std::string &header,
                            std::string &value) const {
  std::lock_guard<std::mutex> lock(_mutex);
  const auto found = _headers.find(header);
  if (found != _headers.end()) {
    value = found->second;
    return true;
  }
  return false;
}

void HttpRequest::Cancel() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_state != HttpRequestState::Error &&
      _state != HttpRequestState::Complete &&
      _state != HttpRequestState::Cancelled) {
    _state = HttpRequestState::Cancelled;
    _response = std::make_shared<HttpResponse>();
    _response->Code = -1;
    _response->Error = "Cancelled";
  }
}

bool HttpRequest::GetResponse(std::shared_ptr<HttpResponse> &response) const {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_state == HttpRequestState::Complete ||
      _state == HttpRequestState::Error ||
      _state == HttpRequestState::Cancelled) {
    response = _response;
    return true;
  }
  return false;
}

HttpRequestState HttpRequest::GetState() const {
  std::lock_guard<std::mutex> lock(_mutex);
  return _state;
}

HttpClient::~HttpClient() {
  _running = false;
  _pollThread.join();
}

HttpClientOptions HttpClient::DEFAULT_OPTIONS{
    .OriginConnectionLimit = 4U,
    .ConnectionTimeout = 10000,
    .KeepAlive = 10000,
};

void HttpClient::SendRequest(std::shared_ptr<HttpRequest> request) {
  {
    std::lock_guard<std::mutex> rLock(request->_mutex);
    if (request->_state == HttpRequestState::Preparing) {
      request->_state = HttpRequestState::Pending;
      std::lock_guard<std::mutex> lock(_mutex);
      _pendingRequests.push_back(request);
    }
  }
}

char tolowerChar(char c) {
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

HttpClient::HttpClient(HttpClientOptions &options)
    : _running(true), _options(options) {
  MemFS::InitMGFS(_fs);
  MemFS::Ensure(S_CA_PEM, &HttpClient::LoadCerts);

  _pollThread = std::thread([this]() {
    mg_mgr mgr;
    mg_mgr_init(&mgr);
    std::vector<std::shared_ptr<HttpRequest>> pending;
    while (_running) {
      std::unique_lock<std::mutex> lock(_mutex);
      pending.clear();
      pending.assign(_pendingRequests.begin(), _pendingRequests.end());
      _pendingRequests.clear();
      lock.unlock();

      // assign all valid pending requests to an origin
      for (const auto &p : pending) {
        std::lock_guard<std::mutex> pLock(p->_mutex);

        std::string url = p->_url;
        std::transform(url.begin(), url.end(), url.begin(), tolowerChar);

        if (!url.starts_with(S_HTTP) && !url.starts_with(S_HTTPS)) {
          p->_state = HttpRequestState::Error;
          p->_response = std::make_shared<HttpResponse>();
          p->_response->Error = "Invalid URL protocol";
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
                                           .MemFS = &_fs})
                       .first;
        }

        p->_url = canonicalURL.str();
        origin->second.PendingRequests.push_back(p);
      }

      mg_mgr_poll(&mgr, 50);

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
              .Request = p,
              .KeepAliveDuration = _options.KeepAlive,
              .ConnectTimeoutDuration = _options.ConnectionTimeout,
              .Origin = &origin.second,
          };
          ++connections;
          mg_http_connect(&mgr, p->_url.c_str(), &HttpClient::HandleResponse,
                          newConnection);
        }
      }
    }
    mg_mgr_free(&mgr);

    // clean up any pending requests or requests in progress
    for (const auto &origin : _origins) {
      for (const auto p : origin.second.PendingRequests) {
        std::lock_guard<std::mutex> lock(p->_mutex);
        p->_state = HttpRequestState::Cancelled;
        p->_response = std::make_shared<HttpResponse>();
        p->_response->Code = -1;
        p->_response->Error = "Cancelled";
      }
      for (auto &c : origin.second.Connections) {
        if (c.second.Request) {
          const auto &p = c.second.Request;
          std::lock_guard<std::mutex> lock(p->_mutex);
          p->_state = HttpRequestState::Cancelled;
          p->_response = std::make_shared<HttpResponse>();
          p->_response->Code = -1;
          p->_response->Error = "Cancelled";
        }
      }
    }
  });
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
  }
  if (ev == MG_EV_POLL) {
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
        conn->Request = request;
        if (!MakeRequestWithConnection(conn)) {
          conn->Request.reset();
          conn->Origin->IdleConnections.insert(std::make_pair(c, conn));
        }
      }
    }
  } else if (ev == MG_EV_CONNECT) {
    if (conn->Origin->UsesTLS) {
      const auto host = mg_url_host(conn->Request->_url.c_str());

      const mg_tls_opts opts = {.ca = S_CA_PEM.c_str(),
                                .srvname = mg_stdstr(conn->Origin->Host),
                                .fs = conn->Origin->MemFS};
      mg_tls_init(c, &opts);
    }
    if (!MakeRequestWithConnection(conn)) {
      conn->Request.reset();
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
    if (conn->KeepAliveDuration > 0) {
      // if keep alives are enabled, keep the connection around for re-use
      conn->Origin->IdleConnections.insert(std::make_pair(c, conn));
    } else {
      c->is_closing = 1;
    }
  } else if (ev == MG_EV_ERROR) {
    if (conn->Request) {
      std::lock_guard<std::mutex> lock(conn->Request->_mutex);
      conn->Request->_state = HttpRequestState::Error;
      conn->Request->_response = std::make_shared<HttpResponse>();
      conn->Request->_response->Error =
          std::string(static_cast<char *>(ev_data));
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
  conn->Request->_state = HttpRequestState::Requesting;

  std::vector<char> compressedBody;
  const char *bodyData = request->_body.data();
  size_t bodyLength = request->_body.size();

  if (!request->_body.empty()) {
    auto found = request->_headers.find(S_CONTENT_ENCODING);
    if (found != request->_headers.end() && found->second == S_GZIP) {
      if (!Resources::CompressString(request->_body, compressedBody)) {
        request->_state = HttpRequestState::Error;
        request->_response = std::make_shared<HttpResponse>();
        request->_response->Error = "Unable to compress request body";
        return false;
      }
      bodyData = compressedBody.data();
      bodyLength = compressedBody.size();
    }
  }

  std::ostringstream headers;
  for (const auto &h : request->_headers) {
    if (h.first == S_ACCEPT_ENCODING || h.first == S_CONTENT_LENGTH) {
      continue;
    }
    headers << h.first << ": " << h.second << "\r\n";
  }

  mg_printf(conn->Connection,
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "%s"
            "Accept-Encoding: gzip\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            request->_method.c_str(), mg_url_uri(request->_url.c_str()),
            conn->Origin->Host.c_str(), headers.str().c_str(), bodyLength);
  mg_send(conn->Connection, bodyData, bodyLength);
  conn->KeepAlive = mg_millis() + conn->KeepAliveDuration;
  return true;
}

void HttpClient::GetResponseFromConnection(HttpConnection *conn,
                                           mg_http_message *hm) {
  std::lock_guard<std::mutex> lock(conn->Request->_mutex);
  const auto request = conn->Request;
  if (request->_state == HttpRequestState::Cancelled) {
    return;
  }

  std::shared_ptr<HttpResponse> response = std::make_shared<HttpResponse>();

  // copy in response code and headers
  response->Code = mg_http_status(hm);
  constexpr const size_t max = sizeof(hm->headers) / sizeof(hm->headers[0]);
  for (size_t i = 0; i < max && hm->headers[i].name.len > 0; i++) {
    struct mg_str *k = &hm->headers[i].name, *v = &hm->headers[i].value;
    response->Headers.insert(std::make_pair(std::string(k->ptr, k->len),
                                            std::string(v->ptr, v->len)));
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

  const auto contentEncodingHeader =
      mg_http_get_header(hm, S_CONTENT_ENCODING.c_str());
  if (contentEncodingHeader) {
    if (mg_strcmp(*contentEncodingHeader, mg_stdstr(S_GZIP)) == 0) {
      Resources::DecompressString(hm->body.ptr, hm->body.len, response->Body);
    } else {
      request->_state = HttpRequestState::Error;
      request->_response = std::make_shared<HttpResponse>();
      request->_response->Error = "Unsupported Content-Encoding";
      return;
    }
  } else {
    response->Body.resize(hm->body.len);
    memcpy_s(response->Body.data(), response->Body.size(), hm->body.ptr,
             hm->body.len);
  }

  request->_response = response;
  request->_state = HttpRequestState::Complete;
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

void HttpClient::LoadCerts(
    std::function<void(const std::string &, const std::string &)> insert) {
  const CERT_ENHKEY_USAGE enhkeyUsage{.cUsageIdentifier = 0,
                                      .rgpszUsageIdentifier = NULL};
  const CERT_USAGE_MATCH certUsage{.dwType = USAGE_MATCH_TYPE_AND,
                                   .Usage = enhkeyUsage};
  CERT_CHAIN_PARA chainParams{.cbSize = sizeof(CERT_CHAIN_PARA),
                              .RequestedUsage = certUsage};

  HCERTSTORE store = CertOpenSystemStoreA(NULL, "ROOT");
  if (!store) {
    return;
  }

  PCCERT_CONTEXT cert = nullptr;
  DWORD size = 0;
  std::ostringstream oss;
  // iterate through all certificates in the trusted root
  while (cert = CertEnumCertificatesInStore(store, cert)) {
    PCCERT_CHAIN_CONTEXT chain;
    if (CertGetCertificateChain(NULL, cert, NULL, NULL, &chainParams, 0, NULL,
                                &chain)) {
      const DWORD errorStatus = chain->TrustStatus.dwErrorStatus;
      CertFreeCertificateChain(chain);
      if (!errorStatus) {
        // if the cert has a valid chain, then add it to our PEM to pass
        // into mbedtls
        CryptBinaryToStringA(cert->pbCertEncoded, cert->cbCertEncoded,
                             CRYPT_STRING_BASE64HEADER, nullptr, &size);
        std::string buffer;
        buffer.resize(static_cast<size_t>(size) -
                      1);  // size includes null-terminator which is
                           // added to the string implicitly
        CryptBinaryToStringA(cert->pbCertEncoded, cert->cbCertEncoded,
                             CRYPT_STRING_BASE64HEADER, buffer.data(), &size);
        oss << buffer;
      }
    }
  }
  CertCloseStore(store, 0);
  // store it in our in memory filesystem cache
  insert(S_CA_PEM, oss.str());
}

std::unordered_map<std::string, std::string> MemFS::_content;
std::mutex MemFS::_mutex;

void MemFS::Ensure(
    const std::string &file,
    std::function<
        void(std::function<void(const std::string &, const std::string &)>)>
        changes) {
  std::lock_guard lock(_mutex);
  if (!_content.contains(file)) {
    changes([](const std::string &file, const std::string &content) {
      _content.insert(std::make_pair(file, content));
    });
  }
}

int MemFS::st(const char *path, size_t *size, time_t *mtime) {
  std::ignore = mtime;
  std::lock_guard lock(_mutex);
  *size = _content.at(path).size();
  return 0;
}

void *MemFS::op(const char *path, int flags) {
  std::ignore = flags;
  std::lock_guard lock(_mutex);
  const auto found = _content.find(path);
  if (found == _content.end()) {
    return nullptr;
  } else {
    return new FD{.Content = found->second, .Offset = 0U};
  }
}

void MemFS::cl(void *fd) { delete static_cast<FD *>(fd); }

size_t MemFS::rd(void *fd, void *buf, size_t len) {
  FD *context = static_cast<FD *>(fd);
  if (context->Offset + len <= context->Content.size()) {
    memcpy(buf, context->Content.data() + context->Offset, len);
    context->Offset += len;
    return len;
  } else {
    const size_t read = context->Content.size() - len;
    if (read != 0) {
      memcpy(buf, context->Content.data() + context->Offset, read);
      context->Offset += len;
    }
    return read;
  }
}

void MemFS::InitMGFS(mg_fs &fs) {
  fs.st = &MemFS::st;
  fs.op = &MemFS::op;
  fs.rd = &MemFS::rd;
  fs.cl = &MemFS::cl;
}

}  // namespace core
}  // namespace MGDF