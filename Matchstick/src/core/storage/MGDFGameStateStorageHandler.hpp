#pragma once

#include <MGDF/MGDFError.hpp>
#include <MGDF/MGDFVersion.hpp>
#include <exception>
#include <string>
#include <unordered_map>
#include <vector>

#include "../common/MGDFVersionHelper.hpp"

namespace MGDF {
namespace core {
namespace storage {

class IGameStateStorageHandler {
 public:
  virtual ~IGameStateStorageHandler() {}
  virtual MGDF::MGDFError Load(const std::wstring &load) = 0;
  virtual void Save(const std::wstring &save) const = 0;

  virtual std::string GetGameUid() const = 0;
  virtual void SetVersion(const MGDF::Version &version) = 0;
  virtual void GetVersion(MGDF::Version &version) const = 0;
  virtual void GetMetadata(
      std::unordered_map<std::string, std::string> &metadata) const = 0;
  virtual void SetMetadata(
      const std::unordered_map<std::string, std::string> &metadata) = 0;
};

}  // namespace storage
}  // namespace core
}  // namespace MGDF