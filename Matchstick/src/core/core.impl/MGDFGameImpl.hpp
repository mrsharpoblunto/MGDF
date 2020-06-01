#pragma once
#include <MGDF/ComObject.hpp>
#include <MGDF/MGDFGame.hpp>
#include <map>
#include <unordered_map>

#include "../common/MGDFLoggerImpl.hpp"
#include "../common/MGDFPreferenceSet.hpp"
#include "../common/MGDFStringImpl.hpp"
#include "../storage/MGDFStorageFactoryComponent.hpp"

namespace MGDF {
namespace core {

/**
this class is the concrete implementation of the configuration interface
*/
class Game : public ComBase<IGame> {
 public:
  Game(const std::string &uid, const std::string &name, const Version &version,
       std::shared_ptr<storage::IStorageFactoryComponent> &storageFactory);
  virtual ~Game(void) {}

  const char *GetUid() const final { return _uid.c_str(); }

  const char *GetName() const final { return _name.c_str(); }

  void GetVersion(Version *version) const final {
    if (version == nullptr) {
      return;
    }
    *version = _version;
  }

  bool HasPreference(const char *name) const final;
  HRESULT GetPreference(const char *name, char *value, size_t *length) final;
  void SetPreference(const char *name, const char *value) final;
  void SetPreferences(IPreferenceSet *preferences) final;
  void SavePreferences() const final;
  void ResetPreferences() final;

  void SavePreferences(const std::wstring &filename);
  MGDFError LoadPreferences(const std::wstring &filename);
  void LoadPreferences(const std::map<std::string, std::string> &preferences);

 private:
  std::shared_ptr<storage::IStorageFactoryComponent> _storageFactory;
  std::string _uid, _name;
  std::wstring _preferencesFile;
  Version _version;
  std::map<std::string, std::string> _preferences;
};

}  // namespace core
}  // namespace MGDF