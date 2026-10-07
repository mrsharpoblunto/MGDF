#include "stdafx.h"

#include "../../src/core/core.impl/MGDFGraphicsRequirements.hpp"

using namespace MGDF::core;

TEST(GraphicsRequirementsTests, PreferenceSelectsListedAPI) {
  const MGDFGraphicsRequirements requirements{
      2,
      {MGDF_GRAPHICS_API_D3D12, MGDF_GRAPHICS_API_D3D11},
      D3D_FEATURE_LEVEL_11_0,
      0};
  MGDFGraphicsAPI api{};
  std::string error;
  EXPECT_EQ(S_OK, SelectGraphicsAPI(requirements, "d3d11", api, error));
  EXPECT_EQ(MGDF_GRAPHICS_API_D3D11, api);
  EXPECT_TRUE(error.empty());
}

TEST(GraphicsRequirementsTests, MissingOrUnlistedPreferenceSelectsFirstAPI) {
  const MGDFGraphicsRequirements requirements{
      1, {MGDF_GRAPHICS_API_D3D11}, D3D_FEATURE_LEVEL_9_3, 0};
  MGDFGraphicsAPI api{};
  std::string error;
  EXPECT_EQ(S_OK, SelectGraphicsAPI(requirements, "", api, error));
  EXPECT_EQ(MGDF_GRAPHICS_API_D3D11, api);
  EXPECT_EQ(S_OK, SelectGraphicsAPI(requirements, "d3d12", api, error));
  EXPECT_EQ(MGDF_GRAPHICS_API_D3D11, api);
}

TEST(GraphicsRequirementsTests, SelectedD3D12IsRejectedWithoutFallingBack) {
  MGDFGraphicsRequirements requirements{
      2,
      {MGDF_GRAPHICS_API_D3D11, MGDF_GRAPHICS_API_D3D12},
      D3D_FEATURE_LEVEL_11_0,
      0x60};
  MGDFGraphicsAPI api{};
  std::string error;
  EXPECT_EQ(E_NOTIMPL, SelectGraphicsAPI(requirements, "d3d12", api, error));
  EXPECT_EQ(MGDF_GRAPHICS_API_D3D12, api);
  EXPECT_EQ("The D3D12 backend is not available in this host version", error);
  requirements.APIs[0] = MGDF_GRAPHICS_API_D3D12;
  requirements.APIs[1] = MGDF_GRAPHICS_API_D3D11;
  EXPECT_EQ(E_NOTIMPL, SelectGraphicsAPI(requirements, "", api, error));
  requirements.APICount = 1;
  EXPECT_EQ(E_NOTIMPL, SelectGraphicsAPI(requirements, "d3d11", api, error));
}

TEST(GraphicsRequirementsTests, UnknownAPIOrPreferenceIsRejected) {
  MGDFGraphicsRequirements requirements{
      1, {MGDF_GRAPHICS_API_D3D11}, D3D_FEATURE_LEVEL_9_3, 0};
  MGDFGraphicsAPI api{};
  std::string error;
  EXPECT_EQ(E_INVALIDARG,
            SelectGraphicsAPI(requirements, "vulkan", api, error));
  EXPECT_FALSE(error.empty());
  requirements.APICount = 2;
  requirements.APIs[1] = static_cast<MGDFGraphicsAPI>(99);
  EXPECT_EQ(E_INVALIDARG, SelectGraphicsAPI(requirements, "d3d11", api, error));
  EXPECT_FALSE(error.empty());
}

TEST(GraphicsRequirementsTests, InvalidAPICountIsRejected) {
  MGDFGraphicsRequirements requirements{};
  MGDFGraphicsAPI api{};
  std::string error;
  EXPECT_EQ(E_INVALIDARG, SelectGraphicsAPI(requirements, "", api, error));
  requirements.APICount = 5;
  EXPECT_EQ(E_INVALIDARG, SelectGraphicsAPI(requirements, "", api, error));
}

TEST(GraphicsRequirementsTests, D3D11LevelsAreDescendingAndRespectMinimum) {
  const std::vector<D3D_FEATURE_LEVEL> expected{
      D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1,
      D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
      D3D_FEATURE_LEVEL_9_3};
  EXPECT_EQ(expected, GetD3D11FeatureLevels(D3D_FEATURE_LEVEL_9_3));
  EXPECT_EQ((std::vector<D3D_FEATURE_LEVEL>{D3D_FEATURE_LEVEL_12_1}),
            GetD3D11FeatureLevels(D3D_FEATURE_LEVEL_12_1));
  EXPECT_EQ(9u, GetD3D11FeatureLevels(D3D_FEATURE_LEVEL_9_1).size());
  EXPECT_TRUE(GetD3D11FeatureLevels(D3D_FEATURE_LEVEL_12_2).empty());
  EXPECT_TRUE(GetD3D11FeatureLevels(static_cast<D3D_FEATURE_LEVEL>(0)).empty());
}
