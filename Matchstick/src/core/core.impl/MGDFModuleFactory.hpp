#pragma once

#include <MGDF/MGDF.h>

namespace MGDF {
namespace core {

typedef HRESULT (*GetCustomArchiveHandlersPtr)(IMGDFArchiveHandler **list,
                                               UINT64 *length, IMGDFLogger *logger);
typedef HRESULT (*GetModulePtr)(IMGDFModule **);
typedef UINT64 (*GetCompatibleFeatureLevelsPtr)(D3D_FEATURE_LEVEL *, UINT64 *);

class ModuleFactory {
 public:
  virtual ~ModuleFactory();
  static HRESULT TryCreate(std::unique_ptr<ModuleFactory> &);

  HRESULT GetCustomArchiveHandlers(IMGDFArchiveHandler **list, UINT64 *length,
                                   IMGDFLogger *logger) const;
  HRESULT GetModule(ComObject<IMGDFModule> &module) const;
  UINT64 GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                    UINT64 *levelSize) const;
  bool GetLastError(std::string &error) const;

 private:
  ModuleFactory();
  HRESULT Init();

  HINSTANCE _moduleInstance;
  GetCustomArchiveHandlersPtr _getCustomArchiveHandlers;
  GetModulePtr _getModule;
  GetCompatibleFeatureLevelsPtr _getCompatibleFeatureLevels;
  std::string _lastError;
};

}  // namespace core
}  // namespace MGDF