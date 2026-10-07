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

extern "C" __declspec(dllexport) HRESULT
GetGraphicsRequirements(MGDFGraphicsRequirements *requirements) {
  if (!requirements) return E_POINTER;
  *requirements = {};
  requirements->APICount = 1;
  requirements->APIs[0] = MGDF_GRAPHICS_API_D3D11;
  requirements->MinFeatureLevel = D3D_FEATURE_LEVEL_9_3;
  return S_OK;
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