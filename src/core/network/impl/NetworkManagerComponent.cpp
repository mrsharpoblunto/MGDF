#include "stdafx.h"

#include "NetworkManagerComponent.hpp"

#include <civetweb.h>
#include <curl/curl.h>

#include <mutex>

#include "../../common/MGDFLoggerImpl.hpp"
#include "NetworkManagerComponentImpl.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace network {
namespace impl {

bool CreateNetworkManagerComponent(
    std::shared_ptr<INetworkManagerComponent> &network,
    const NetworkManagerOptions &options) {
  // libcurl and civetweb global state is initialized once for the
  // lifetime of the process
  static bool initialized = false;
  static std::once_flag initFlag;
  std::call_once(initFlag, []() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
      LOG("Unable to initialize libcurl", MGDF_LOG_ERROR);
      return;
    }
    mg_init_library(0);
    initialized = true;
  });
  if (!initialized) {
    return false;
  }
  network = std::make_shared<NetworkManagerComponent>(options);
  return true;
}

}  // namespace impl
}  // namespace network
}  // namespace core
}  // namespace MGDF
