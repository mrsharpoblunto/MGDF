#include "stdafx.h"

#include "MGDFHttpCommon.hpp"

#include <sstream>

#include "MGDFCertificates.hpp"
#include "MGDFMongooseMemFS.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

static const std::string S_CONTENT_ENCODING("Content-Encoding");
static const std::string S_ACCEPT_ENCODING("Accept-Encoding");
static const std::string S_CONTENT_LENGTH("Content-Length");
static const std::string S_GZIP("gzip");

namespace MGDF {
namespace core {

NetworkEventLoop::NetworkEventLoop() : _running(true) {
  MemFS::InitMGFS(_fs);
  MemFS::Ensure(CertificateManager::S_CA_PEM, &CertificateManager::LoadCerts);
  _thread = std::thread([this]() {
    mg_mgr_init(&_mgr);
    while (_running) {
      mg_mgr_poll(&_mgr, 1000);
      {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto l : _listeners) {
          l->OnPoll(_mgr, _fs);
        }
      }
    }
    mg_mgr_free(&_mgr);
  });
}

NetworkEventLoop::~NetworkEventLoop() {
  _running = false;
  _thread.join();
}

void NetworkEventLoop::Add(INetworkEventListener *l) {
  std::lock_guard<std::mutex> lock(_mutex);
  _listeners.insert(l);
}

void NetworkEventLoop::Remove(INetworkEventListener *l) {
  std::lock_guard<std::mutex> lock(_mutex);
  _listeners.erase(l);
}

void SendHttpRequest(struct mg_connection *c, const std::string &host,
                     const std::string &url, HttpMessageBase &m) {
  std::vector<char> compressedBody;
  const char *bodyData = m.Body.data();
  size_t bodyLength = m.Body.size();

  if (!m.Body.empty()) {
    auto found = m.Headers.find(S_CONTENT_ENCODING);
    if (found != m.Headers.end() && found->second == S_GZIP) {
      if (!Resources::CompressString(m.Body, compressedBody)) {
        m.Error = "Unable to compress request body";
        m.Body.clear();
      }
      bodyData = compressedBody.data();
      bodyLength = compressedBody.size();
    }
  }

  std::ostringstream headers;
  for (const auto &h : m.Headers) {
    if (h.first == S_ACCEPT_ENCODING || h.first == S_CONTENT_LENGTH) {
      continue;
    }
    headers << h.first << ": " << h.second << "\r\n";
  }

  mg_printf(c,
            "%s %s HTTP/1.1\r\n"
            "Host: %s\r\n"
            "%s"
            "Accept-Encoding: gzip\r\n"
            "Content-Length: %d\r\n"
            "\r\n",
            m.Method.c_str(), mg_url_uri(url.c_str()), host.c_str(),
            headers.str().c_str(), bodyLength);
  mg_send(c, bodyData, bodyLength);
}

void SendHttpResponse(struct mg_connection *c, const HttpMessageBase &m) {
  std::vector<char> compressedBody;
  const char *bodyData = m.Body.data();
  size_t bodyLength = m.Body.size();

  if (!m.Body.empty()) {
    auto found = m.Headers.find(S_CONTENT_ENCODING);
    if (found != m.Headers.end() && found->second == S_GZIP) {
      if (!Resources::CompressString(m.Body, compressedBody)) {
        mg_http_reply(c, 500, NULL, "\n");
        return;
      }
      bodyData = compressedBody.data();
      bodyLength = compressedBody.size();
    }
  }

  std::ostringstream headers;
  for (const auto &h : m.Headers) {
    if (h.first == S_ACCEPT_ENCODING || h.first == S_CONTENT_LENGTH) {
      continue;
    }
    headers << h.first << ": " << h.second << "\r\n";
  }

  mg_printf(c,
            "HTTP/1.1 %d %s\r\n"
            "%s"
            "Content-Length %d\r\n"
            "\r\n",
            m.Code, headers.str().c_str(), bodyLength);
  mg_send(c, bodyData, bodyLength);
}

}  // namespace core
}  // namespace MGDF
