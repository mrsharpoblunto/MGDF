#pragma once

#include "../MGDFPreferenceConfigStorageHandler.hpp"

namespace MGDF {
namespace core {
namespace storage {
namespace json {

class JsonPreferenceConfigStorageHandler
    : public IPreferenceConfigStorageHandler {
 public:
  JsonPreferenceConfigStorageHandler() {}
  virtual ~JsonPreferenceConfigStorageHandler() {}

  void Add(const std::string &name, const std::string &value) final;
  IPreferenceConfigStorageHandler::iterator begin() const final;
  IPreferenceConfigStorageHandler::iterator end() const final;

  HRESULT Load(const std::wstring &) final;
  void Save(const std::wstring &) const final;

 private:
  std::map<std::string, std::string> _preferences;
};

}  // namespace json
}  // namespace storage
}  // namespace core
}  // namespace MGDF