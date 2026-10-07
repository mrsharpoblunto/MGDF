#include "StdAfx.h"

#include "MGDFGraphicsRequirements.hpp"

namespace MGDF {
namespace core {

HRESULT SelectGraphicsAPI(const MGDFGraphicsRequirements &requirements,
                          const std::string &preference, MGDFGraphicsAPI &api,
                          std::string &error) {
  error.clear();
  if (requirements.APICount == 0 || requirements.APICount > 4) {
    error = "GetGraphicsRequirements must list between 1 and 4 graphics APIs";
    return E_INVALIDARG;
  }
  for (UINT32 i = 0; i < requirements.APICount; ++i) {
    if (requirements.APIs[i] != MGDF_GRAPHICS_API_D3D11 &&
        requirements.APIs[i] != MGDF_GRAPHICS_API_D3D12) {
      error = "GetGraphicsRequirements lists an unknown graphics API";
      return E_INVALIDARG;
    }
  }
  if (!preference.empty() && preference != "d3d11" && preference != "d3d12") {
    error = "Unknown host.graphicsAPI preference: " + preference;
    return E_INVALIDARG;
  }

  api = requirements.APIs[0];
  if (!preference.empty()) {
    const auto preferred = preference == "d3d11" ? MGDF_GRAPHICS_API_D3D11
                                                 : MGDF_GRAPHICS_API_D3D12;
    for (UINT32 i = 0; i < requirements.APICount; ++i) {
      if (requirements.APIs[i] == preferred) {
        api = preferred;
        break;
      }
    }
  }
  return S_OK;
}

std::vector<D3D_FEATURE_LEVEL> GetD3D11FeatureLevels(
    D3D_FEATURE_LEVEL minimum) {
  constexpr D3D_FEATURE_LEVEL supported[] = {
      D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
      D3D_FEATURE_LEVEL_9_3,  D3D_FEATURE_LEVEL_9_2,  D3D_FEATURE_LEVEL_9_1};
  std::vector<D3D_FEATURE_LEVEL> levels;
  for (const auto level : supported) {
    if (level >= minimum) levels.push_back(level);
    if (level == minimum) return levels;
  }
  return {};
}

}  // namespace core
}  // namespace MGDF
