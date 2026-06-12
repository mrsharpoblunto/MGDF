#pragma once

#include "../MGDFNetworkManagerComponent.hpp"

namespace MGDF {
namespace core {
namespace network {
namespace impl {

bool CreateNetworkManagerComponent(
    std::shared_ptr<INetworkManagerComponent> &network,
    const NetworkManagerOptions &options);

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
