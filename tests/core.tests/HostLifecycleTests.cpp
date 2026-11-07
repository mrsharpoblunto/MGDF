#include "stdafx.h"

#include <filesystem>

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
