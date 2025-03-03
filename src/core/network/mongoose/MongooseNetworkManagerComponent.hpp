#pragma once

#include "../MGDFNetworkManagerComponent.hpp"

namespace MGDF {
namespace core {
namespace network {
namespace mongoose {

bool CreateNetworkManagerComponent(
    std::shared_ptr<INetworkManagerComponent> &network,
    const NetworkManagerOptions &options);

}  // namespace mongoose
}  // namespace network
}  // namespace core
}  // namespace MGDF
