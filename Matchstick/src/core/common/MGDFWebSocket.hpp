#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "mongoose.h"

namespace MGDF {
namespace core {

struct WebSocketMessage {
  WebSocketMessage(size_t size, bool binary) : Data(size), Binary(binary) {}
  std::vector<char> Data;
  bool Binary;
};

class WebSocketClient {
 public:
  WebSocketClient(const std::string &url);
  virtual ~WebSocketClient();

  void Send(const std::vector<char> &data, bool binary = false);
  void Send(void *data, size_t dataLength, bool binary = false);
  bool Receive(std::vector<char> &message);

  MGDFWebSocketConnectionState GetConnectionState(std::string &lastError);

 private:
  static void HandleRequest(struct mg_connection *c, int ev, void *ev_data,
                            void *fn_data);

  struct mg_fs _fs;
  struct mg_mgr _mgr;
  struct mg_connection *_conn;

  bool _usesTLS;
  std::string _url;
  bool _running;
  uint64_t _disconnected;
  std::thread _pollThread;
  std::deque<std::vector<char>> _in;
  std::vector<WebSocketMessage> _out;
  std::string _lastError;
  MGDFWebSocketConnectionState _state;
  std::mutex _mutex;
};

class WebSocketServer {
 public:
  WebSocketServer();
  virtual ~WebSocketServer();

  void Listen(const std::string &port);
  bool Listening() const { return _conn != nullptr; }
  void Send(const std::vector<char> &data, bool binary = false);
  virtual void OnRequest(struct mg_connection *c, struct mg_ws_message *m) = 0;

 private:
  static void HandleRequest(struct mg_connection *c, int ev, void *ev_data,
                            void *fn_data);
  struct mg_mgr _mgr;
  struct mg_connection *_conn;
  bool _running;
  std::thread _pollThread;
  std::mutex _mutex;
};

}  // namespace core
}  // namespace MGDF
