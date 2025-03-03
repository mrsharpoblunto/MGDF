#pragma once

#include <MGDF/MGDF.h>

#include "../MGDFStorageFactoryComponent.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace json {

bool CreateStorageFactoryComponent(
    std::shared_ptr<IStorageFactoryComponent> &comp);

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF