#pragma once
#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>
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
class Game : public ComBase<IMGDFGame> {
 public:
  Game(const std::string &uid, const std::string &name,
       const std::string &statisticsService, const MGDFVersion &version,
       std::shared_ptr<storage::IStorageFactoryComponent> &storageFactory);
  virtual ~Game(void) {}

  const char *__stdcall GetUid() final { return _uid.c_str(); }
  const char *__stdcall GetName() final { return _name.c_str(); }

  void __stdcall GetVersion(MGDFVersion *version) final {
    if (version == nullptr) {
      return;
    }
    *version = _version;
  }

  BOOL __stdcall HasPreference(const char *name) final;
  HRESULT __stdcall GetPreference(const char *name, char *value,
                                  UINT64 *length) final;
  void __stdcall SetPreference(const char *name, const char *value) final;
  void __stdcall SetPreferences(IMGDFPreferenceSet *preferences) final;
  void __stdcall SavePreferences() final;
  void __stdcall ResetPreferences() final;

  void SavePreferences(const std::wstring &filename);
  HRESULT LoadPreferences(const std::wstring &filename);
  void LoadPreferences(const std::map<std::string, std::string> &preferences);
  const char *GetStatististicsService() const {
    return _statisticsService.c_str();
  }

 private:
  std::shared_ptr<storage::IStorageFactoryComponent> _storageFactory;
  std::string _uid, _name;
  std::wstring _preferencesFile;
  std::string _statisticsService;
  MGDFVersion _version;
  std::map<std::string, std::string> _preferences;
};

}  // namespace core
}  // namespace MGDF