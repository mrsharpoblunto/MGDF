#include "stdafx.h"

#include <filesystem>
#include <future>

#include "../../src/core/common/MGDFResources.hpp"
#include "../../src/core/core.impl/MGDFHostImpl.hpp"
#include "../../src/core/network/impl/NetworkManagerComponent.hpp"
#include "../../src/core/storage/jsoncpp/JsonStorageFactoryComponent.hpp"

using namespace MGDF;
using namespace MGDF::core;

class HostLifecycleIntegrationTests : public ::testing::Test {
 protected:
  void SetUp() override {
    Resources::Instance(GetModuleHandleW(nullptr));
    std::string name;
    ASSERT_TRUE(GuidString(name));
    Resources::Instance().SetUserBaseDir("host-tests-" + name, true);
    _root = Resources::Instance().UserBaseDir();
    std::filesystem::create_directories(Resources::Instance().SaveBaseDir());
    ASSERT_TRUE(
        storage::json::CreateStorageFactoryComponent(_components.Storage));
    ASSERT_TRUE(network::impl::CreateNetworkManagerComponent(
        _components.Network, {4, 1, 1, 1}));
    const auto game = MakeCom<Game>("host-tests", "Host tests", "",
                                    MGDFVersion{}, _components.Storage);
    _host = MakeCom<Host>(game, _components);
  }

  void TearDown() override {
    _host.Clear();
    _components.Network.reset();
    std::error_code error;
    std::filesystem::remove_all(_root, error);
    Resources::Instance().SetUserBaseDir("junkship", true);
  }

  std::filesystem::path _root;
  HostComponents _components;
  ComObject<Host> _host;
};

TEST_F(HostLifecycleIntegrationTests, ShutdownWithoutAudio) { _host.Clear(); }

TEST_F(HostLifecycleIntegrationTests, RecursiveFatalErrorTerminates) {
  EXPECT_EXIT(
      {
        _host->SetFatalErrorHandler(
            [&](const std::string &, const std::string &) {
              _host->FatalError("test", "recursive failure");
            });
        _host->FatalError("test", "first failure");
      },
      ::testing::ExitedWithCode(1), "");
}

namespace {
class StatisticsNetwork : public network::INetworkManagerComponent {
 public:
  explicit StatisticsNetwork(
      std::shared_ptr<network::INetworkManagerComponent> network)
      : _network(std::move(network)) {}
  void Stop() final { _network->Stop(); }
  std::unique_ptr<network::IHttpClientRequest> CreateHttpRequest(
      const std::string &url) final {
    if (!_received.exchange(true)) Requested.set_value(url);
    return _network->CreateHttpRequest(url);
  }
  std::shared_ptr<network::IHttpServer> CreateHttpServer(
      uint32_t port, const std::string &path) final {
    return _network->CreateHttpServer(port, path);
  }
  std::shared_ptr<network::IWebSocket> CreateWebSocket(
      const std::string &url) final {
    return _network->CreateWebSocket(url);
  }
  std::promise<std::string> Requested;

 private:
  std::shared_ptr<network::INetworkManagerComponent> _network;
  std::atomic<bool> _received{false};
};
}  // namespace

TEST_F(HostLifecycleIntegrationTests, ManifestStatisticsNeedNoPermissionFlag) {
  _host.Clear();
  // Capture the HTTP boundary. The host and statistics manager remain real.
  auto network = std::make_shared<StatisticsNetwork>(_components.Network);
  auto requested = network->Requested.get_future();
  _components.Network = network;
  const auto game =
      MakeCom<Game>("host-tests", "Host tests", "http://127.0.0.1:1/statistics",
                    MGDFVersion{}, _components.Storage);
  _host = MakeCom<Host>(game, _components);
  ComObject<IMGDFStatisticsManager> statistics;
  _host->GetStatistics(statistics.Assign());
  for (int i = 0; i < 25; ++i) statistics->PushString("test", "value", nullptr);
  ASSERT_EQ(std::future_status::ready,
            requested.wait_for(std::chrono::seconds(10)));
  EXPECT_EQ("http://127.0.0.1:1/statistics", requested.get());
}
