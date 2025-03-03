#include "stdafx.h"

#include "MongooseNetworkManagerComponent.hpp"

#include "../../common/MGDFLoggerImpl.hpp"
#include "MongooseNetworkManagerComponentImpl.hpp"
#include "mongoose.h"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {
namespace network {
namespace mongoose {

void MongooseLog(unsigned char ch) {
  static char buffer[256];
  static size_t len;
  buffer[len++] = ch;
  if (ch == '\n' || len >= sizeof(buffer)) {
    buffer[len - 1] = '\0';  // Null-terminate the string
    Logger::Instance().Log("Mongoose", buffer, MGDF_LOG_HIGH);
    len = 0;
  }
}

bool CreateNetworkManagerComponent(
    std::shared_ptr<INetworkManagerComponent> &network,
    const NetworkManagerOptions &options) {
  mg_log_set_fn(MongooseLog);
  network = std::make_shared<MongooseNetworkManagerComponent>(options);
  return true;
}

}  // namespace mongoose
}  // namespace network
}  // namespace core
}  // namespace MGDF