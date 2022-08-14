#include "StdAfx.h"

#include <MGDF/MGDF.h>

#include "FakeArchiveHandler.hpp"
#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

BOOL APIENTRY DllMain(HMODULE const hModule, DWORD const ul_reason_for_call,
                      LPVOID const lpReserved) {
  std::ignore = lpReserved;
  std::ignore = ul_reason_for_call;
  std::ignore = hModule;
  return TRUE;
}

// create module instances as they are requested by the framework
extern "C" __declspec(dllexport) HRESULT GetModule(IMGDFModule **module) {
  auto m = MGDF::MakeCom<MGDF::Test::Module>();
  m.AddRawRef(module);
  return S_OK;
}

// specify to the framework what kind of d3d device features we want/require
extern "C" __declspec(dllexport) UINT64
    GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,
                               UINT64 *featureLevelsSize) {
  if (*featureLevelsSize != 1) {
    *featureLevelsSize = 1;
    return 1;
  } else {
    levels[0] = D3D_FEATURE_LEVEL_9_3;
    return 0;
  }
}

// register custom archive handlers
extern "C" __declspec(dllexport) HRESULT
    GetCustomArchiveHandlers(IMGDFArchiveHandler **list, UINT64 *length,
                             IMGDFLogger *logger) {
  *length = 1;
  if (!list) {
    return S_OK;
  }

  if (*length >= 1) {
    auto handler = MGDF::MakeCom<MGDF::Test::FakeArchiveHandler>(logger);
    handler.AddRawRef(list);
    return S_OK;
  }
  return E_FAIL;
}