#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "mongoose.h"

namespace MGDF {
namespace core {

struct WebSocketMessage {
  WebSocketMessage(size_t size, bool binary) : Data(size), Binary(binary) {}
  std::vector<char> Data;
  bool Binary;
};

class WebSocketConnection {
 public:
  WebSocketConnection(unsigned int id);
  virtual ~WebSocketConnection();

  void Send(const std::vector<char> &data, bool binary = false);
  void Send(void *data, size_t dataLength, bool binary = false);
  bool Receive(std::vector<char> &message);

  MGDFWebSocketConnectionState GetConnectionState(std::string &lastError);
  unsigned int GetId() const { return _id; }

  void Poll(struct mg_connection *c, int ev, void *ev_data, void *fn_data);

 protected:
  unsigned int _id;
  std::deque<std::vector<char>> _in;
  std::vector<WebSocketMessage> _out;
  std::string _lastError;
  MGDFWebSocketConnectionState _state;
  std::mutex _mutex;
};

class WebSocketClient : public WebSocketConnection {
 public:
  WebSocketClient(const std::string &url);
  virtual ~WebSocketClient();

 private:
  static void HandleRequest(struct mg_connection *c, int ev, void *ev_data,
                            void *fn_data);

  struct mg_fs _fs;
  struct mg_mgr _mgr;
  struct mg_connection *_conn;

  uint64_t _disconnected;
  bool _usesTLS;
  std::string _url;
  bool _running;
  std::thread _pollThread;
};

class WebSocketServer {
 public:
  WebSocketServer();
  virtual ~WebSocketServer();

  void Listen(const std::string &port);
  bool Listening() const { return _conn != nullptr; }
  virtual void OnConnected(
      std::shared_ptr<WebSocketConnection> &connection) = 0;

 private:
  static void HandleRequest(struct mg_connection *c, int ev, void *ev_data,
                            void *fn_data);
  struct mg_mgr _mgr;
  struct mg_connection *_conn;
  bool _running;
  std::thread _pollThread;
  std::mutex _mutex;

  std::unordered_map<unsigned int, std::shared_ptr<WebSocketConnection>>
      _connections;
};

}  // namespace core
}  // namespace MGDF
