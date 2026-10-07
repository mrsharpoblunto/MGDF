#pragma once

#include <MGDF/MGDF.h>

#include <string>
#include <vector>

namespace MGDF {
namespace core {

HRESULT SelectGraphicsAPI(const MGDFGraphicsRequirements &requirements,
                          const std::string &preference, MGDFGraphicsAPI &api,
                          std::string &error);
std::vector<D3D_FEATURE_LEVEL> GetD3D11FeatureLevels(D3D_FEATURE_LEVEL minimum);

}  // namespace core
}  // namespace MGDF
