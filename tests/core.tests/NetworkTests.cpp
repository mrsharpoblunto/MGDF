#include "stdafx.h"

#include <future>

#include "../../src/core/network/impl/NetworkManagerComponentImpl.hpp"

using namespace MGDF::core::network;
using namespace MGDF::core::network::impl;

namespace {
class Signal {
 public:
  Signal() : _future(_promise.get_future()) {}
  void Set() { _promise.set_value(); }
  bool Wait() {
    return _future.wait_for(std::chrono::seconds(5)) ==
           std::future_status::ready;
  }

 private:
  std::promise<void> _promise;
  std::future<void> _future;
};
constexpr NetworkManagerOptions Options{4, 1, 1, 0};
}  // namespace

TEST(NetworkTests, SocketReleaseDuringConnectDoesNotWaitForThePollThread) {
  Signal entered, resume;
  std::atomic<UINT32> released{0};
  const auto manager = std::make_shared<NetworkManagerComponent>(
      Options, WebSocketTransport{[&]() {
                                    entered.Set();
                                    EXPECT_TRUE(resume.Wait());
                                    return curl_easy_init();
                                  },
                                  [&](CURL *connection) {
                                    curl_easy_cleanup(connection);
                                    ++released;
                                  }});
  auto socket = manager->CreateWebSocket("ws://127.0.0.1:1");
  EXPECT_TRUE(entered.Wait());
  socket.reset();
  resume.Set();
  manager->Stop();
  EXPECT_EQ(1U, released.load());
}

TEST(NetworkTests, SocketStateSurvivesUserReleaseDuringTransportCleanup) {
  Signal cleaning, resume;
  std::atomic<UINT32> released{0};
  const auto manager = std::make_shared<NetworkManagerComponent>(
      Options, WebSocketTransport{curl_easy_init, [&](CURL *connection) {
                                    cleaning.Set();
                                    EXPECT_TRUE(resume.Wait());
                                    curl_easy_cleanup(connection);
                                    ++released;
                                  }});
  auto socket = manager->CreateWebSocket("ws://");
  EXPECT_TRUE(cleaning.Wait());
  auto release = std::async(std::launch::async, [&]() { socket.reset(); });
  EXPECT_EQ(std::future_status::ready,
            release.wait_for(std::chrono::seconds(5)));
  resume.Set();
  release.get();
  manager->Stop();
  EXPECT_EQ(1U, released.load());
}

TEST(NetworkTests, StopCancelsQueuedRequestsOutsideTheAdmissionLock) {
  Signal connecting, resume, cancelled;
  const auto manager = std::make_shared<NetworkManagerComponent>(
      Options, WebSocketTransport{[&]() {
        connecting.Set();
        EXPECT_TRUE(resume.Wait());
        return curl_easy_init();
      }});
  const auto socket = manager->CreateWebSocket("ws://127.0.0.1:1");
  EXPECT_TRUE(connecting.Wait());
  std::atomic<UINT32> callbacks{0};
  auto request = manager->CreateHttpRequest("http://127.0.0.1:1");
  const auto pending = request->SendRequest([&](auto &response) {
    EXPECT_EQ("Cancelled", response->Error);
    const auto nested =
        manager->CreateHttpRequest("http://127.0.0.1:1")->SendRequest(nullptr);
    EXPECT_EQ(HttpRequestState::Cancelled, nested->GetRequestState());
    ++callbacks;
    cancelled.Set();
  });
  auto stop = std::async(std::launch::async, [&]() { manager->Stop(); });
  EXPECT_TRUE(cancelled.Wait());
  resume.Set();
  EXPECT_EQ(std::future_status::ready, stop.wait_for(std::chrono::seconds(5)));
  stop.get();
  EXPECT_EQ(HttpRequestState::Cancelled, pending->GetRequestState());
  EXPECT_EQ(1U, callbacks.load());
}

TEST(NetworkTests, RequestsAfterStopReturnACancelledResponse) {
  const auto manager = std::make_shared<NetworkManagerComponent>(Options);
  auto request = manager->CreateHttpRequest("http://127.0.0.1:1");
  manager->Stop();
  UINT32 callbacks = 0;
  const auto pending = request->SendRequest([&](auto &response) {
    EXPECT_EQ("Cancelled", response->Error);
    ++callbacks;
  });
  EXPECT_EQ(HttpRequestState::Cancelled, pending->GetRequestState());
  std::shared_ptr<HttpMessage> response;
  ASSERT_TRUE(pending->GetResponse(response));
  EXPECT_EQ(-1, response->Code);
  EXPECT_EQ("Cancelled", response->Error);
  pending->CancelRequest();
  EXPECT_EQ(1U, callbacks);
}
