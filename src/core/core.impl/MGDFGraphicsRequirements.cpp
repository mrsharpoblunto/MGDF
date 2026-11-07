#include "StdAfx.h"

#include "MGDFGraphicsRequirements.hpp"

#include <cmath>

namespace MGDF {
namespace core {

HRESULT ValidateGraphicsRequirements(
    const MGDFGraphicsRequirements &requirements, std::string &error) {
  error.clear();
  if (requirements.HDRFormat != MGDF_HDR_FORMAT_SCRGB &&
      requirements.HDRFormat != MGDF_HDR_FORMAT_HDR10) {
    error = "GetGraphicsRequirements lists an unknown HDR format";
    return E_INVALIDARG;
  }
  return S_OK;
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
