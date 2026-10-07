#pragma once

#include <MGDF/MGDF.h>

#include <MGDF/ComObject.hpp>

namespace MGDF {
namespace core {

typedef HRESULT (*GetCustomArchiveHandlersPtr)(IMGDFArchiveHandler **list,
                                               UINT64 *length,
                                               IMGDFLogger *logger);
typedef HRESULT (*GetModulePtr)(IMGDFModule **);
typedef HRESULT (*GetGraphicsRequirementsPtr)(MGDFGraphicsRequirements *);

class ModuleFactory {
 public:
  virtual ~ModuleFactory();
  static HRESULT TryCreate(std::unique_ptr<ModuleFactory> &);

  HRESULT GetCustomArchiveHandlers(IMGDFArchiveHandler **list, UINT64 *length,
                                   IMGDFLogger *logger) const;
  HRESULT GetModule(ComObject<IMGDFModule> &module) const;
  HRESULT GetGraphicsRequirements(MGDFGraphicsRequirements &requirements) const;
  bool GetLastError(std::string &error) const;

 private:
  ModuleFactory();
  HRESULT Init();

  HINSTANCE _moduleInstance;
  GetCustomArchiveHandlersPtr _getCustomArchiveHandlers;
  GetModulePtr _getModule;
  GetGraphicsRequirementsPtr _getGraphicsRequirements;
  std::string _lastError;
};

}  // namespace core
}  // namespace MGDF