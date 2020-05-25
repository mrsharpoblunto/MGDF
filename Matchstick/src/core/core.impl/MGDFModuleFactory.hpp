#pragma once

#include <MGDF/MGDF.hpp>

namespace MGDF {
namespace core {

typedef HRESULT (*GetCustomArchiveHandlersPtr)(IArchiveHandler **list,
                                               UINT32 *length, ILogger *logger);
typedef HRESULT (*GetModulePtr)(IModule **);
typedef UINT32 (*GetCompatibleFeatureLevelsPtr)(D3D_FEATURE_LEVEL *, UINT32 *);

class ModuleFactory {
 public:
  virtual ~ModuleFactory();
  static HRESULT TryCreate(std::unique_ptr<ModuleFactory> &);

  HRESULT GetCustomArchiveHandlers(IArchiveHandler **list, UINT32 *length,
                                   ILogger *logger) const;
  HRESULT GetModule(ComObject<IModule> &module) const;
  UINT32 GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                    UINT32 *levelSize) const;
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