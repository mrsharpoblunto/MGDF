#pragma once

#include <MGDF/MGDF.h>
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
  virtual void GetVersion(MGDFVersion &version) const = 0;
  virtual HRESULT Load(const std::wstring &load) = 0;
  virtual const std::map<std::string, std::string> &GetPreferences() const = 0;
};

}  // namespace storage
}  // namespace core
}  // namespace MGDF