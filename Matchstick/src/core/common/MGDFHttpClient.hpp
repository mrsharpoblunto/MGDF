#pragma once

#include <json/json.h>

#include <mutex>
#include <string>
#include <unordered_map>

#include "mongoose.h"

namespace MGDF {
namespace core {

class HttpClient {
 public:
  HttpClient();
  virtual ~HttpClient();

  int PostJson(const std::string &url, const Json::Value &request,
               size_t timeout = 10000);

  int GetJson(const std::string &url, Json::Value &response,
              size_t timeout = 10000);

  std::string GetLastError() const { return _lastError; }

 private:
  int RequestJson(const std::string &url, const Json::Value *request,
                  Json::Value *response, size_t timeout);

  static void HandleResponse(struct mg_connection *c, int ev, void *ev_data,
                             void *fn_data);

  static void LoadCerts();

  struct mg_mgr _mgr;
  bool _running;
  size_t _timeout;
  std::string _url;
  const Json::Value *_request;
  Json::Value *_response;
  int _responseCode;
  std::string _lastError;
};

// mongoose has a virtual filsystem, so this is an in-memory
// implementation to populate with SSL certs as needed
class MemFS {
 public:
  static bool Contains(const std::string &file);
  static void Insert(const std::string &file, const std::string &content);
  static std::string Get(const std::string &file);
  static void InitMGFS(mg_fs &fs);

 private:
  struct FD {
    std::string &Content;
    size_t Offset;
  };
  static std::mutex _mutex;
  static std::unordered_map<std::string, std::string> _content;
  static mg_fs _mgfs;

  static int st(const char *path, size_t *size, time_t *mtime);
  static void *op(const char *path, int flags);
  static void cl(void *fd);
  static size_t rd(void *fd, void *buf, size_t len);  // Read file
};

}  // namespace core
}  // namespace MGDF
