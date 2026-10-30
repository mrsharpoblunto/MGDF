#include "stdafx.h"

#include "../../src/core/MGDFD3D12FrameState.hpp"
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

TEST(GraphicsRequirementsTests, SelectsD3D12WithoutFallingBack) {
  MGDFGraphicsRequirements requirements{
      2,
      {MGDF_GRAPHICS_API_D3D11, MGDF_GRAPHICS_API_D3D12},
      D3D_FEATURE_LEVEL_11_0,
      0x60};
  MGDFGraphicsAPI api{};
  std::string error;
  EXPECT_EQ(S_OK, SelectGraphicsAPI(requirements, "d3d12", api, error));
  EXPECT_EQ(MGDF_GRAPHICS_API_D3D12, api);
  EXPECT_TRUE(error.empty());
  requirements.APIs[0] = MGDF_GRAPHICS_API_D3D12;
  requirements.APIs[1] = MGDF_GRAPHICS_API_D3D11;
  EXPECT_EQ(S_OK, SelectGraphicsAPI(requirements, "", api, error));
  requirements.APICount = 1;
  EXPECT_EQ(S_OK, SelectGraphicsAPI(requirements, "d3d11", api, error));
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

TEST(GraphicsRequirementsTests, OnlyKnownHDRFormatsAreAccepted) {
  MGDFGraphicsRequirements requirements{1,
                                        {MGDF_GRAPHICS_API_D3D12},
                                        D3D_FEATURE_LEVEL_11_0,
                                        0,
                                        MGDF_HDR_FORMAT_HDR10};
  MGDFGraphicsAPI api{};
  std::string error;
  EXPECT_EQ(S_OK, SelectGraphicsAPI(requirements, "", api, error));
  requirements.HDRFormat = static_cast<MGDFHDRFormat>(2);
  EXPECT_EQ(E_INVALIDARG, SelectGraphicsAPI(requirements, "", api, error));
  EXPECT_FALSE(error.empty());
}

TEST(GraphicsRequirementsTests, HDRMetadataConvertsToDXGIUnits) {
  DXGI_HDR_METADATA_HDR10 hdr10{};
  ASSERT_EQ(S_OK, ToHDR10Metadata({.MaxContentLightLevel = 900.4f,
                                   .MaxFrameAverageLightLevel = 400.6f,
                                   .MaxMasteringLuminance = 1000.0f,
                                   .MinMasteringLuminance = 0.005f},
                                  hdr10));
  EXPECT_EQ(900u, hdr10.MaxContentLightLevel);
  EXPECT_EQ(401u, hdr10.MaxFrameAverageLightLevel);
  EXPECT_EQ(1000u, hdr10.MaxMasteringLuminance);
  EXPECT_EQ(50u, hdr10.MinMasteringLuminance);
  // Rec.709 and D65 in 0.00002 units.
  EXPECT_EQ(32000u, hdr10.RedPrimary[0]);
  EXPECT_EQ(16500u, hdr10.RedPrimary[1]);
  EXPECT_EQ(15000u, hdr10.GreenPrimary[0]);
  EXPECT_EQ(30000u, hdr10.GreenPrimary[1]);
  EXPECT_EQ(7500u, hdr10.BluePrimary[0]);
  EXPECT_EQ(3000u, hdr10.BluePrimary[1]);
  EXPECT_EQ(15635u, hdr10.WhitePoint[0]);
  EXPECT_EQ(16450u, hdr10.WhitePoint[1]);
}

TEST(GraphicsRequirementsTests, HDRMetadataOutOfRangeIsRejected) {
  const MGDFHDRMetadata valid{.MaxContentLightLevel = 1000.0f,
                              .MaxFrameAverageLightLevel = 400.0f,
                              .MaxMasteringLuminance = 1000.0f,
                              .MinMasteringLuminance = 0.0f};
  const auto rejects = [](MGDFHDRMetadata metadata) {
    DXGI_HDR_METADATA_HDR10 hdr10{};
    return ToHDR10Metadata(metadata, hdr10) == E_INVALIDARG;
  };
  EXPECT_FALSE(rejects(valid));
  auto bad = valid;
  bad.MinMasteringLuminance = bad.MaxMasteringLuminance;
  EXPECT_TRUE(rejects(bad));
  bad = valid;
  bad.MaxMasteringLuminance = 10001.0f;
  EXPECT_TRUE(rejects(bad));
  bad = valid;
  bad.MaxMasteringLuminance = 0.0f;
  bad.MinMasteringLuminance = 0.0f;
  EXPECT_TRUE(rejects(bad));
  bad = valid;
  bad.MaxContentLightLevel = 65536.0f;
  EXPECT_TRUE(rejects(bad));
  bad = valid;
  bad.MaxFrameAverageLightLevel = 1001.0f;
  EXPECT_TRUE(rejects(bad));
  bad = valid;
  bad.MinMasteringLuminance = -1.0f;
  EXPECT_TRUE(rejects(bad));
  bad = valid;
  bad.MaxContentLightLevel = std::numeric_limits<float>::quiet_NaN();
  EXPECT_TRUE(rejects(bad));
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

TEST(GraphicsRequirementsTests, D3D12AdapterScoring) {
  const D3D12AdapterScore low{false, 1024};
  const D3D12AdapterScore high{false, 8192};
  EXPECT_TRUE(PreferD3D12Adapter(false, true, {}, std::nullopt));
  EXPECT_FALSE(PreferD3D12Adapter(true, true, high, low));
  EXPECT_FALSE(PreferD3D12Adapter(false, false, high, low));
  EXPECT_FALSE(PreferD3D12Adapter(false, true, low, low));
  EXPECT_FALSE(PreferD3D12Adapter(false, true, {false, 512}, low));
  EXPECT_TRUE(PreferD3D12Adapter(false, true, high, low));
}

TEST(GraphicsRequirementsTests, D3D12AdapterWithRayTracingWinsOverMemory) {
  const D3D12AdapterScore rayTracing{true, 1024};
  const D3D12AdapterScore high{false, 8192};
  EXPECT_TRUE(PreferD3D12Adapter(false, true, rayTracing, high));
  EXPECT_FALSE(PreferD3D12Adapter(false, true, high, rayTracing));
  EXPECT_TRUE(PreferD3D12Adapter(false, true, {true, 2048}, rayTracing));
}

TEST(GraphicsRequirementsTests, FrameSlotsWaitForTheirOwnPreviousSubmission) {
  D3D12FrameState frames;
  EXPECT_EQ(1u, frames.Ordinal());
  EXPECT_EQ(1u, frames.Slot());
  EXPECT_EQ(0u, frames.PreviousFence());
  frames.Submitted();
  EXPECT_EQ(2u, frames.Ordinal());
  EXPECT_EQ(0u, frames.Slot());
  EXPECT_EQ(0u, frames.PreviousFence());
  frames.Submitted();
  for (UINT64 ordinal = 3; ordinal < 10000; ++ordinal) {
    EXPECT_EQ(ordinal, frames.Ordinal());
    EXPECT_EQ(ordinal % 2, frames.Slot());
    EXPECT_EQ(ordinal - 2, frames.PreviousFence());
    frames.Submitted();
  }
  frames = {};
  EXPECT_EQ(1u, frames.Ordinal());
  EXPECT_EQ(0u, frames.PreviousFence());
}
