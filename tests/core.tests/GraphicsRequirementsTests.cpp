#include "stdafx.h"

#include "../../src/core/MGDFD3D12FrameState.hpp"
#include "../../src/core/core.impl/MGDFGraphicsRequirements.hpp"

using namespace MGDF::core;

TEST(GraphicsRequirementsTests, OnlyKnownHDRFormatsAreAccepted) {
  MGDFGraphicsRequirements requirements{D3D_FEATURE_LEVEL_11_0, 0,
                                        MGDF_HDR_FORMAT_HDR10};
  std::string error;
  EXPECT_EQ(S_OK, ValidateGraphicsRequirements(requirements, error));
  requirements.HDRFormat = static_cast<MGDFHDRFormat>(2);
  EXPECT_EQ(E_INVALIDARG, ValidateGraphicsRequirements(requirements, error));
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
