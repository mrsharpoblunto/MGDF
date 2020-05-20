#pragma once

#include <MGDF/MGDFError.hpp>
#include <MGDF/MGDFVersion.hpp>
#include <exception>
#include <map>
#include <string>

namespace MGDF {
namespace core {
namespace storage {

class IGameStorageHandler {
 public:
  virtual ~IGameStorageHandler() {}
  virtual std::string GetGameName() const = 0;
  virtual std::string GetGameUid() const = 0;
  virtual void GetVersion(Version &version) const = 0;
  virtual INT32 GetInterfaceVersion() const = 0;
  virtual MGDFError Load(const std::wstring &load) = 0;
  virtual const std::map<std::string, std::string> &GetPreferences() const = 0;
};

}  // namespace storage
}  // namespace core
}  // namespace MGDF