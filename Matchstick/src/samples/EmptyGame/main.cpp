#include "StdAfx.h"

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  return TRUE;
}

// specify to the framework what kind of d3d device features we want/require
extern "C" __declspec(dllexport) UINT64 GetCompatibleFeatureLevels(D3D_FEATURE_LEVEL *levels,
                                        UINT64 *featureLevelsSize) {
  // if you want to use the default feature support levels
  // just set featureLevelsSize to 0 and return 0

  // if you want to support a specific feature set (in this example 9_3 only)
  // then first assert the levels array passed in is of the correct size
  if (*featureLevelsSize != 1) {
    // ensure featureLevelsSize is set to the same value as the return value
    *featureLevelsSize = 1;

    // return how large the featureLevels array should be
    return 1;
  } else {
    // specify all the features in descending order of preference
    levels[0] = D3D_FEATURE_LEVEL_9_3;

    // then return 0
    return 0;
  }
}

// create a module instance when requested by the host
extern "C" __declspec(dllexport) HRESULT GetModule(IMGDFModule **module) {
  auto m = MGDF::MakeCom<Module>();
  m.AddRawRef(module);
  return S_OK;
}

// register custom archive handlers
extern "C" __declspec(dllexport) HRESULT GetCustomArchiveHandlers(IMGDFArchiveHandler **list, UINT64 *length,
                                       IMGDFLogger *logger) {
  *length = 0;
  return S_OK;
}