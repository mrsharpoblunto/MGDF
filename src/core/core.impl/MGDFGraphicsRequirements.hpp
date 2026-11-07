#pragma once

#include <MGDF/MGDF.h>
#include <dxgi1_5.h>

#include <string>
#include <vector>

namespace MGDF {
namespace core {

// What the current device supports. Empty before the device exists.
struct GraphicsSupport {
  std::vector<MGDFUpscaleMode> UpscaleModes;
  UINT32 MaxGeneratedFrames = 0;
  // Always starts with MGDF_LOW_LATENCY_OFF.
  std::vector<MGDFLowLatencyMode> LowLatencyModes{MGDF_LOW_LATENCY_OFF};
  bool RayTracing = false;
  bool ShaderExecutionReordering = false;
};

HRESULT ValidateGraphicsRequirements(
    const MGDFGraphicsRequirements &requirements, std::string &error);
// Returns E_INVALIDARG for values outside the MGDFHDRMetadata ranges.
HRESULT ToHDR10Metadata(const MGDFHDRMetadata &metadata,
                        DXGI_HDR_METADATA_HDR10 &hdr10);

}  // namespace core
}  // namespace MGDF
