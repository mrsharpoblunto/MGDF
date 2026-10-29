#pragma once

#include <MGDF/MGDF.h>
#include <dxgi1_5.h>

#include <string>
#include <vector>

namespace MGDF {
namespace core {

HRESULT SelectGraphicsAPI(const MGDFGraphicsRequirements &requirements,
                          const std::string &preference, MGDFGraphicsAPI &api,
                          std::string &error);
std::vector<D3D_FEATURE_LEVEL> GetD3D11FeatureLevels(D3D_FEATURE_LEVEL minimum);
// Returns E_INVALIDARG for values outside the MGDFHDRMetadata ranges.
HRESULT ToHDR10Metadata(const MGDFHDRMetadata &metadata,
                        DXGI_HDR_METADATA_HDR10 &hdr10);

}  // namespace core
}  // namespace MGDF
