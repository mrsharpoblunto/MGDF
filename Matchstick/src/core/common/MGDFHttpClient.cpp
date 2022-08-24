#include "stdafx.h"

#include "MGDFHttpClient.hpp"

#include <Windows.h>
#include <wincrypt.h>

#include <algorithm>
#include <sstream>
#include <string_view>

#include "MGDFResources.hpp"

#pragma warning(disable : 4706)

namespace MGDF {
namespace core {

std::mutex MemFS::_mutex;
std::unordered_map<std::string, std::string> MemFS::_content;
mg_fs MemFS::_mgfs;

bool MemFS::Contains(const std::string &file) {
  std::lock_guard lock(_mutex);
  return _content.contains(file);
}
void MemFS::Insert(const std::string &file, const std::string &content) {
  std::lock_guard lock(_mutex);
  _content.insert(std::make_pair(file, content));
}
std::string MemFS::Get(const std::string &file) {
  std::lock_guard lock(_mutex);
  return _content.at(file);
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
  auto found = _content.find(path);
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

HttpClient::HttpClient()
    : _running(false),
      _request(nullptr),
      _response(nullptr),
      _responseCode(0),
      _timeout(0) {}
HttpClient::~HttpClient() {}

int HttpClient::PostJson(const std::string &url, const Json::Value &content,
                         size_t timeout) {
  return RequestJson(url, &content, nullptr, timeout);
}

int HttpClient::GetJson(const std::string &url, Json::Value &content,
                        size_t timeout) {
  return RequestJson(url, nullptr, &content, timeout);
}

int HttpClient::RequestJson(const std::string &url, const Json::Value *request,
                            Json::Value *response, size_t timeout) {
  _running = true;
  _timeout = timeout;
  _url = url;
  _request = request;
  _response = response;
  _responseCode = 0;
  _lastError.clear();

  mg_mgr_init(&_mgr);
  mg_http_connect(&_mgr, url.c_str(), &HttpClient::HandleResponse, this);
  while (_running) mg_mgr_poll(&_mgr, 50);
  mg_mgr_free(&_mgr);

  return _responseCode;
}

void HttpClient::LoadCerts() {
  CERT_ENHKEY_USAGE enhkeyUsage{.cUsageIdentifier = 0,
                                .rgpszUsageIdentifier = NULL};
  CERT_USAGE_MATCH certUsage{.dwType = USAGE_MATCH_TYPE_AND,
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
        // if the cert has a valid chain, then add it to our PEM to pass into
        // mbedtls
        CryptBinaryToStringA(cert->pbCertEncoded, cert->cbCertEncoded,
                             CRYPT_STRING_BASE64HEADER, nullptr, &size);
        std::string buffer;
        buffer.resize(size - 1);  // size includes null-terminator which is
                                  // added to the string implicitly
        CryptBinaryToStringA(cert->pbCertEncoded, cert->cbCertEncoded,
                             CRYPT_STRING_BASE64HEADER, buffer.data(), &size);
        oss << buffer;
      }
    }
  }
  CertCloseStore(store, 0);
  // store it in our in memory filesystem cache
  MemFS::Insert("ca.pem", oss.str());
}

void HttpClient::HandleResponse(struct mg_connection *c, int ev, void *ev_data,
                                void *fn_data) {
  HttpClient *client = static_cast<HttpClient *>(fn_data);
  if (ev == MG_EV_OPEN) {
    // Connection created. Store connect expiration time in c->label
    *(uint64_t *)c->label = mg_millis() + client->_timeout;
  } else if (ev == MG_EV_POLL) {
    if (mg_millis() > *(uint64_t *)c->label &&
        (c->is_connecting || c->is_resolving)) {
      mg_error(c, "Connect timeout");
    }
  } else if (ev == MG_EV_CONNECT) {
    // Connected to server. Extract host name from URL
    struct mg_str host = mg_url_host(client->_url.c_str());

    if (mg_url_is_ssl(client->_url.c_str())) {
      // make sure the mbedtls in memory filesystem has the certs we need
      if (!MemFS::Contains("ca.pem")) {
        client->LoadCerts();
      }
      mg_fs fs;
      MemFS::InitMGFS(fs);
      mg_tls_opts opts = {.ca = "ca.pem", .srvname = host, .fs = &fs};
      mg_tls_init(c, &opts);
    }

    const bool isPost = client->_request != nullptr;
    const char *method = isPost ? "POST" : "GET";
    std::vector<char> body;
    if (isPost) {
      std::ostringstream oss;
      oss << *client->_request;
      if (!Resources::CompressString(oss.str(), body)) {
        mg_error(c, "Failed to compress request body");
        return;
      }
    }

    mg_printf(c,
              "%s %s HTTP/1.0\r\n"
              "Host: %.*s\r\n"
              "Content-Type: application/json\r\n"
              "Content-Encoding: gzip\r\n"
              "Accept-Encoding: gzip\r\n"
              "Accept: application/json\r\n"
              "Content-Length: %d\r\n"
              "\r\n",
              method, mg_url_uri(client->_url.c_str()), (int)host.len, host.ptr,
              body.size());
    mg_send(c, body.data(), body.size());
  } else if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    client->_responseCode = mg_http_status(hm);
    if (client->_response) {
      const auto contentTypeHeader = mg_http_get_header(hm, "Content-Type");
      if (!contentTypeHeader) {
        mg_error(c, "No Content-Type header found");
        return;
      }
      const std::string_view contentType(contentTypeHeader->ptr,
                                         contentTypeHeader->len);
      if (!contentType.starts_with("application/json") &&
          !contentType.starts_with("text/javascript")) {
        mg_error(c, "Response is not valid Json");
        return;
      }
      Json::Reader reader;

      const auto contentEncodingHeader =
          mg_http_get_header(hm, "Content-Encoding");
      if (contentEncodingHeader) {
        const std::string_view contentEncoding(contentEncodingHeader->ptr,
                                               contentEncodingHeader->len);
        if (contentEncoding == "gzip") {
          std::string inflated;
          Resources::DecompressString(hm->body.ptr, hm->body.len, inflated);
          if (!reader.parse(inflated, *client->_response)) {
            mg_error(c, "Response is not valid Json");
            return;
          }
        } else {
          mg_error(c, "Unsupported Content-Encoding");
          return;
        }
      } else {
        if (!reader.parse(hm->body.ptr, hm->body.ptr + hm->body.len,
                          *client->_response)) {
          mg_error(c, "Response is not valid Json");
          return;
        }
      }
    }
    c->is_closing = 1;         // Tell mongoose to close this connection
    client->_running = false;  // Tell event loop to stop
  } else if (ev == MG_EV_ERROR) {
    client->_lastError = std::string(static_cast<char *>(ev_data));
    client->_running = false;  // Tell event loop to stop
  }
}

}  // namespace core
}  // namespace MGDF