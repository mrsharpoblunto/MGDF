#include "stdafx.h"

#include <DirectXPackedVector.h>

#include <MGDF/ComObject.hpp>
#include <array>
#include <filesystem>

#include "../../src/core/core.impl/MGDFCaptures.hpp"

using namespace MGDF;
using namespace MGDF::core;

namespace {

std::array<uint8_t, 4> Convert(DXGI_FORMAT format, CaptureEncoding encoding,
                               float white, const void *pixel, uint32_t pitch) {
  std::array<uint8_t, 4> rgba{};
  EXPECT_TRUE(ConvertToRGBA8(static_cast<const uint8_t *>(pixel), pitch, 1, 1,
                             format, encoding, white, rgba.data()));
  return rgba;
}

uint32_t PackR10G10B10(uint32_t r, uint32_t g, uint32_t b) {
  return r | (g << 10) | (b << 20) | (3u << 30);
}

// ST 2084 reference points: 100 nits is code 520, 1000 nits is code 769
constexpr uint32_t kPQ100 = 520;
constexpr uint32_t kPQ1000 = 769;

std::array<uint16_t, 4> Half(float value) {
  const auto h = DirectX::PackedVector::XMConvertFloatToHalf(value);
  return {h, h, h, DirectX::PackedVector::XMConvertFloatToHalf(1.0f)};
}

}  // namespace

TEST(CaptureTests, NamesStayInsideTheCaptureFolder) {
  const std::filesystem::path folder = L"C:/user/captures";
  EXPECT_EQ(folder / L"shot.png", CapturePath(folder, "shot", L".png"));
  EXPECT_EQ(folder / L"shot.PNG", CapturePath(folder, "shot.PNG", L".png"));
  EXPECT_EQ(folder / L"waves" / L"cliff.mp4",
            CapturePath(folder, "waves/cliff", L".mp4"));
  EXPECT_EQ(folder / L"a" / L"b.mp4", CapturePath(folder, "a\\b", L".mp4"));
  for (const char *name : {"", "../out", "a/../b", "/root", "C:/out", "a//b",
                           "a/", "shot.", "bad|name", "tab\tname"}) {
    EXPECT_FALSE(CapturePath(folder, name, L".png")) << name;
  }
}

TEST(CaptureTests, ConvertsBackBufferFormatsToSRGB) {
  const uint8_t bgra[] = {10, 20, 30, 0};
  EXPECT_EQ((std::array<uint8_t, 4>{30, 20, 10, 255}),
            Convert(DXGI_FORMAT_B8G8R8A8_UNORM, CaptureEncoding::SDR, 80.0f,
                    bgra, 4));

  // HDR10: the SDR white level is white, half of it is sRGB 0.5, more clips
  const uint32_t white = PackR10G10B10(kPQ100, kPQ100, kPQ100);
  const auto mapped = Convert(DXGI_FORMAT_R10G10B10A2_UNORM,
                              CaptureEncoding::HDR10, 100.0f, &white, 4);
  EXPECT_NEAR(mapped[0], 255, 2);
  EXPECT_NEAR(mapped[1], 255, 2);
  const uint32_t bright = PackR10G10B10(kPQ1000, kPQ1000, kPQ1000);
  EXPECT_EQ(255, Convert(DXGI_FORMAT_R10G10B10A2_UNORM, CaptureEncoding::HDR10,
                         100.0f, &bright, 4)[2]);
  EXPECT_NEAR(188,
              Convert(DXGI_FORMAT_R10G10B10A2_UNORM, CaptureEncoding::HDR10,
                      200.0f, &white, 4)[0],
              2);

  // scRGB: 1.0 is 80 nits
  const auto twoHundred = Half(2.5f);
  EXPECT_EQ(255, Convert(DXGI_FORMAT_R16G16B16A16_FLOAT, CaptureEncoding::ScRGB,
                         200.0f, twoHundred.data(), 8)[0]);
  const auto half = Half(1.25f);
  EXPECT_NEAR(188,
              Convert(DXGI_FORMAT_R16G16B16A16_FLOAT, CaptureEncoding::ScRGB,
                      200.0f, half.data(), 8)[1],
              1);

  std::array<uint8_t, 4> unused{};
  EXPECT_FALSE(ConvertToRGBA8(bgra, 4, 1, 1, DXGI_FORMAT_R32_FLOAT,
                              CaptureEncoding::SDR, 80.0f, unused.data()));
}

TEST(CaptureTests, ConvertsToRec709LimitedRangeNV12) {
  struct Case {
    uint8_t R, G, B;
    uint8_t Y, U, V;
  };
  for (const auto &c :
       {Case{255, 255, 255, 235, 128, 128}, Case{0, 0, 0, 16, 128, 128},
        Case{255, 0, 0, 63, 102, 240}}) {
    std::array<uint8_t, 16> rgba{};
    for (size_t i = 0; i < 4; ++i) {
      rgba[i * 4] = c.R;
      rgba[i * 4 + 1] = c.G;
      rgba[i * 4 + 2] = c.B;
    }
    std::array<uint8_t, 6> nv12{};
    ConvertToNV12(rgba.data(), 8, 2, 2, nv12.data());
    EXPECT_EQ((std::array<uint8_t, 6>{c.Y, c.Y, c.Y, c.Y, c.U, c.V}), nv12);
  }
}
