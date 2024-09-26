#pragma once

#include <functional>
#include <string>

namespace MGDF {
namespace core {

class CertificateManager {
 public:
  static const std::string S_CA_PEM;

  static void LoadCerts(
      std::function<void(const std::string &, const std::string &)> insert);
};

}  // namespace core
}  // namespace MGDF
