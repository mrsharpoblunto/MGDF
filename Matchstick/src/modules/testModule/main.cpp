#include "StdAfx.h"

#include <MGDF/MGDF.hpp>

#include "FakeArchiveHandler.hpp"
#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  return TRUE;
}

// Function that returns if this module is compatible with the framework calling
// it
bool MGDF::IsCompatibleInterfaceVersion(INT32 interfaceVersion) {
  return interfaceVersion == 1;  // compatible with v1 interface
}

// create module instances as they are requested by the framework
HRESULT MGDF::GetModule(IModule **module) {
  ComObject<IModule> m(new MGDF::Test::Module());
  m.AddRawRef(module);
  return S_OK;
}

// specify to the framework what kind of d3d device features we want/require
UINT32 MGDF::GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                        UINT32 *featureLevelsSize) {
  if (*featureLevelsSize != 1) {
    *featureLevelsSize = 1;
    return 1;
  } else {
    levels[0] = D3D_FEATURE_LEVEL_9_3;
    return 0;
  }
}

// register custom archive handlers
bool MGDF::GetCustomArchiveHandlers(IArchiveHandler **list, UINT32 *length,
                                    ILogger *logger,
                                    IErrorHandler *errorHandler) {
  if (*length >= 1) {
    ComObject<IArchiveHandler> handler(
        new MGDF::Test::FakeArchiveHandler(logger, errorHandler));
    handler.AddRawRef(list);
    *length = 1;
    return true;
  }
  *length = 1;
  return false;
}