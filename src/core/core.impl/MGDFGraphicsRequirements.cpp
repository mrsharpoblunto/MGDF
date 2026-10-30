#include "StdAfx.h"

#include "MGDFGraphicsRequirements.hpp"

#include <cmath>

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
  constexpr UINT32 knownFeatures = MGDF_GRAPHICS_FEATURE_UPSCALE |
                                   MGDF_GRAPHICS_FEATURE_FRAME_GENERATION |
                                   MGDF_GRAPHICS_FEATURE_LOW_LATENCY;
  if (requirements.Features & ~knownFeatures) {
    error = "GetGraphicsRequirements lists an unknown graphics feature";
    return E_INVALIDARG;
  }
  if (requirements.HDRFormat != MGDF_HDR_FORMAT_SCRGB &&
      requirements.HDRFormat != MGDF_HDR_FORMAT_HDR10) {
    error = "GetGraphicsRequirements lists an unknown HDR format";
    return E_INVALIDARG;
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

HRESULT ToHDR10Metadata(const MGDFHDRMetadata &metadata,
                        DXGI_HDR_METADATA_HDR10 &hdr10) {
  const auto in = [](float value, float minimum, float maximum) {
    return std::isfinite(value) && value >= minimum && value <= maximum;
  };
  if (!in(metadata.MaxContentLightLevel, 0.0f, 65535.0f) ||
      !in(metadata.MaxFrameAverageLightLevel, 0.0f,
          metadata.MaxContentLightLevel) ||
      !in(metadata.MaxMasteringLuminance, 0.0f, 10000.0f) ||
      metadata.MaxMasteringLuminance <= 0.0f ||
      !in(metadata.MinMasteringLuminance, 0.0f,
          metadata.MaxMasteringLuminance) ||
      metadata.MinMasteringLuminance >= metadata.MaxMasteringLuminance)
    return E_INVALIDARG;
  // Chromaticity is in units of 0.00002: Rec.709 primaries and D65.
  hdr10 = {.RedPrimary = {32000, 16500},
           .GreenPrimary = {15000, 30000},
           .BluePrimary = {7500, 3000},
           .WhitePoint = {15635, 16450},
           .MaxMasteringLuminance =
               static_cast<UINT>(std::lround(metadata.MaxMasteringLuminance)),
           .MinMasteringLuminance = static_cast<UINT>(
               std::lround(metadata.MinMasteringLuminance * 10000.0f)),
           .MaxContentLightLevel =
               static_cast<UINT16>(std::lround(metadata.MaxContentLightLevel)),
           .MaxFrameAverageLightLevel = static_cast<UINT16>(
               std::lround(metadata.MaxFrameAverageLightLevel))};
  return S_OK;
}

}  // namespace core
}  // namespace MGDF
