#pragma once

#include <MGDF/MGDF.h>
#include <dxgi1_5.h>

#include <string>
#include <vector>

namespace MGDF {
namespace core {

// What the current device supports. Empty before the device exists and with
// D3D11.
struct GraphicsSupport {
  std::vector<MGDFUpscaleMode> UpscaleModes;
  UINT32 MaxGeneratedFrames = 0;
  // Always starts with MGDF_LOW_LATENCY_OFF.
  std::vector<MGDFLowLatencyMode> LowLatencyModes{MGDF_LOW_LATENCY_OFF};
  bool RayTracing = false;
  bool ShaderExecutionReordering = false;
};

HRESULT SelectGraphicsAPI(const MGDFGraphicsRequirements &requirements,
                          const std::string &preference, MGDFGraphicsAPI &api,
                          std::string &error);
std::vector<D3D_FEATURE_LEVEL> GetD3D11FeatureLevels(D3D_FEATURE_LEVEL minimum);
// Returns E_INVALIDARG for values outside the MGDFHDRMetadata ranges.
HRESULT ToHDR10Metadata(const MGDFHDRMetadata &metadata,
                        DXGI_HDR_METADATA_HDR10 &hdr10);

}  // namespace core
}  // namespace MGDF
