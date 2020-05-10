#pragma once

#include "MGDFHostImpl.hpp"

namespace MGDF {
namespace core {

class HostBuilder {
 public:
  static MGDFError TryCreateHost(Host** host);
  static void DisposeHost(Host*);

 private:
  static bool RegisterBaseComponents();
  static bool RegisterAdditionalComponents(std::string gameUid);
  static void UnregisterComponents();
  static void InitParameterManager();
  static void InitResources(std::string gameUid = "");
  static void InitLogger();
  static std::string GetApplicationDirectory(HINSTANCE instance);
};

}  // namespace core
}  // namespace MGDF