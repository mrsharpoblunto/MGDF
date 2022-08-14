#pragma once

#include <string>
#include <thread>

#include "mongoose.h"

namespace MGDF {
namespace common {

class HttpServer {
 public:
  HttpServer();
  virtual ~HttpServer();

  void Listen(const std::string &port);
  bool Listening() const { return _conn != nullptr; }
  virtual void OnRequest(struct mg_connection *c,
                         struct mg_http_message *m) = 0;

 private:
  static void HandleRequest(struct mg_connection *c, int ev, void *ev_data,
                            void *fn_data);
  struct mg_mgr _mgr;
  struct mg_connection *_conn;
  bool _running;
  std::thread _pollThread;
};

class HttpClient {};

}  // namespace common
}  // namespace MGDF
