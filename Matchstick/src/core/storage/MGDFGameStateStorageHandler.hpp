#pragma once

#include <MGDF/MGDF.h>

#include <string>
#include <unordered_map>

namespace MGDF {
namespace core {
namespace storage {

class IGameStateStorageHandler {
 public:
  virtual ~IGameStateStorageHandler() {}
  virtual HRESULT Load(const std::wstring &load) = 0;
  virtual void Save(const std::wstring &save) const = 0;

  virtual std::string GetGameUid() const = 0;
  virtual void SetVersion(const MGDFVersion &version) = 0;
  virtual void GetVersion(MGDFVersion &version) const = 0;
  virtual void GetMetadata(
      std::unordered_map<std::string, std::string> &metadata) const = 0;
  virtual void SetMetadata(
      const std::unordered_map<std::string, std::string> &metadata) = 0;
};

}  // namespace storage
}  // namespace core
}  // namespace MGDF