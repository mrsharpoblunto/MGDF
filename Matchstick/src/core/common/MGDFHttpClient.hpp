#pragma once

#include <json/json.h>

#include <string>

#include "mongoose.h"

namespace MGDF {
namespace core {

class HttpClient {
 public:
  HttpClient();
  virtual ~HttpClient();

  int PostJson(const std::string &url, const Json::Value &content,
               size_t timeout = 10000);

  std::string GetLastError() const { return _lastError; }

 private:
  static void HandleResponse(struct mg_connection *c, int ev, void *ev_data,
                             void *fn_data);
  struct mg_mgr _mgr;
  bool _running;
  size_t _timeout;
  std::string _url;
  const Json::Value *_content;
  int _responseCode;
  std::string _lastError;
};

}  // namespace core
}  // namespace MGDF
