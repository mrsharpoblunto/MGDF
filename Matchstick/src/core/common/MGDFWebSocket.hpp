#pragma once

#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "MGDFHttpCommon.hpp"
#include "mongoose.h"

namespace MGDF {
namespace core {

struct WebSocketMessage {
  WebSocketMessage(size_t size, bool binary) : Data(size), Binary(binary) {}
  std::vector<char> Data;
  bool Binary;
};

class WebSocketConnectionBase {
 public:
  WebSocketConnectionBase(mg_connection *c, MGDFWebSocketConnectionState state);
  virtual ~WebSocketConnectionBase();

  void Send(const std::vector<char> &data, bool binary = false);
  void Send(void *data, size_t dataLength, bool binary = false);
  bool Receive(std::vector<char> &message, bool &binary);

  MGDFWebSocketConnectionState GetConnectionState(std::string &lastError);
  unsigned int GetId() const { return _conn ? _conn->id : 0; }

 protected:
  void Poll(int ev, void *ev_data, void *fn_data);

  struct mg_connection *_conn;
  std::deque<std::pair<std::vector<char>, bool>> _in;
  std::vector<WebSocketMessage> _out;
  std::string _lastError;
  MGDFWebSocketConnectionState _state;
  std::mutex _mutex;
};

class WebSocketClientConnection : public INetworkEventListener,
                                  public WebSocketConnectionBase {
 public:
  WebSocketClientConnection(std::shared_ptr<NetworkEventLoop> &eventLoop,
                            const std::string &url);
  virtual ~WebSocketClientConnection();

  void OnPoll(mg_mgr &mgr, mg_fs &fs) final;

 private:
  static void HandleRequest(struct mg_connection *c, int ev, void *ev_data,
                            void *fn_data);

  struct mg_fs *_fs;
  struct mg_connection *_conn;

  uint64_t _disconnected;
  bool _usesTLS;
  std::string _url;
  std::shared_ptr<NetworkEventLoop> _eventLoop;
};

}  // namespace core
}  // namespace MGDF
