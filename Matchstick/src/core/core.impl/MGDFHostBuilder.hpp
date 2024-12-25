#pragma once

#include "MGDFHostImpl.hpp"

namespace MGDF {
namespace core {

class HostBuilder {
 public:
  static HRESULT TryCreateHost(ComObject<Host> &host);

 private:
  static bool RegisterBaseComponents(HostComponents &);
  static bool RegisterAdditionalComponents(std::string gameUid,
                                           HostComponents &);
  static void InitParameterManager();
  static void InitResources(std::string gameUid = "");
  static void InitLogger(const std::shared_ptr<NetworkEventLoop> &eventLoop);
};

}  // namespace core
}  // namespace MGDF