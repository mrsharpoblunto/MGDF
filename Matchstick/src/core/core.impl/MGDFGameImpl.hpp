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
  Game(const std::string &uid, const std::string &name, INT32 interfaceVersion,
       const Version &version, storage::IStorageFactoryComponent *xmlFactory);
  virtual ~Game(void) {}

  const char *GetUid() const override final { return _uid.c_str(); }

  const char *GetName() const override final { return _name.c_str(); }

  INT32 GetInterfaceVersion() const override final { return _interfaceVersion; }

  void GetVersion(Version *version) const override final {
    *version = _version;
  }

  bool HasPreference(const char *name) const override final;
  bool GetPreference(const char *name, IString **value) override final;
  void SetPreference(const char *name, const char *value) override final;
  void SetPreferences(IPreferenceSet *preferences) override final;
  void SavePreferences() const override final;
  void ResetPreferences() override final;

  void SavePreferences(const std::wstring &filename);
  MGDFError LoadPreferences(const std::wstring &filename);
  void LoadPreferences(const std::map<std::string, std::string> &preferences);

 private:
  storage::IStorageFactoryComponent *_storageFactory;
  std::string _uid, _name;
  std::wstring _preferencesFile;
  Version _version;
  INT32 _interfaceVersion;
  std::map<std::string, std::string> _preferences;
};

}  // namespace core
}  // namespace MGDF