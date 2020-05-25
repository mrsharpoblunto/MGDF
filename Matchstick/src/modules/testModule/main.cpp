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
HRESULT MGDF::GetCustomArchiveHandlers(IArchiveHandler **list, UINT32 *length,
                                       ILogger *logger) {
  *length = 1;
  if (!list) {
    return S_OK;
  }

  if (*length >= 1) {
    ComObject<IArchiveHandler> handler(
        new MGDF::Test::FakeArchiveHandler(logger));
    handler.AddRawRef(list);
    return S_OK;
  }
  return E_FAIL;
}