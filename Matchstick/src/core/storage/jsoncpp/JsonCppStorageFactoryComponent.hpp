#pragma once

#include <MGDF/MGDF.h>

#include "../MGDFStorageFactoryComponent.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace jsoncppImpl {

bool CreateStorageFactoryComponent(
    std::shared_ptr<IStorageFactoryComponent> &comp);

}  // namespace jsoncppImpl
}  // namespace storage
}  // namespace core
}  // namespace MGDF