#include "StdAfx.h"

#include "Module.hpp"

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call,
                      LPVOID lpReserved) {
  return TRUE;
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