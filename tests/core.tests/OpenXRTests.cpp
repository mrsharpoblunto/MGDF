#include "stdafx.h"

#include <DirectXMath.h>

#include "../../src/core/core.impl/MGDFOpenXR.hpp"

using namespace MGDF::core;
using namespace DirectX;

TEST(OpenXRTests, PoseReflectsPositionAndRotationIntoLeftHandedSpace) {
  XrView eye{XR_TYPE_VIEW};
  eye.pose.position = {0.25f, 1.6f, -2.0f};
  XMFLOAT4 quaternion;
  XMStoreFloat4(&quaternion,
                XMQuaternionRotationRollPitchYaw(0.3f, -0.7f, 0.2f));
  eye.pose.orientation = {quaternion.x, quaternion.y, quaternion.z,
                          quaternion.w};
  const auto view =
      ConvertOpenXRView(eye, 1000, 1200, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
  EXPECT_FLOAT_EQ(0.25f, view.Position[0]);
  EXPECT_FLOAT_EQ(1.6f, view.Position[1]);
  EXPECT_FLOAT_EQ(2.0f, view.Position[2]);
  // Reflect each rotated basis vector, not just the quaternion components.
  const auto converted = XMVectorSet(view.Orientation[0], view.Orientation[1],
                                     view.Orientation[2], view.Orientation[3]);
  for (const auto direction : {XMVectorSet(1, 0, 0, 0), XMVectorSet(0, 1, 0, 0),
                               XMVectorSet(0, 0, -1, 0)}) {
    const auto reflect = XMVectorSet(1, 1, -1, 1);
    const auto expected = XMVectorMultiply(
        XMVector3Rotate(direction, XMLoadFloat4(&quaternion)), reflect);
    const auto actual =
        XMVector3Rotate(XMVectorMultiply(direction, reflect), converted);
    EXPECT_NEAR(XMVectorGetX(expected), XMVectorGetX(actual), 0.00001f);
    EXPECT_NEAR(XMVectorGetY(expected), XMVectorGetY(actual), 0.00001f);
    EXPECT_NEAR(XMVectorGetZ(expected), XMVectorGetZ(actual), 0.00001f);
  }
}

TEST(OpenXRTests, EyeFormatPrefersSRGBAndSkipsUnsupportedFormats) {
  const auto renderable = [](DXGI_FORMAT format) {
    return format != DXGI_FORMAT_D32_FLOAT;
  };
  const std::vector<int64_t> formats{
      DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,
      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB};
  EXPECT_EQ(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            SelectOpenXRFormat(formats, renderable));
  EXPECT_EQ(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,
            SelectOpenXRFormat(formats, [](auto format) {
              return format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
            }));
  EXPECT_EQ(
      DXGI_FORMAT_R16G16B16A16_FLOAT,
      SelectOpenXRFormat(
          {DXGI_FORMAT_D32_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT}, renderable));
  EXPECT_EQ(DXGI_FORMAT_UNKNOWN,
            SelectOpenXRFormat({DXGI_FORMAT_D32_FLOAT}, renderable));
}

TEST(OpenXRTests, SessionStatesSeparateEndingTheFrameLoopFromFallback) {
  EXPECT_EQ(OpenXRAction::Wait, GetOpenXRAction(XR_SESSION_STATE_IDLE));
  EXPECT_EQ(OpenXRAction::Start, GetOpenXRAction(XR_SESSION_STATE_READY));
  for (const auto state : {XR_SESSION_STATE_SYNCHRONIZED,
                           XR_SESSION_STATE_VISIBLE, XR_SESSION_STATE_FOCUSED})
    EXPECT_EQ(OpenXRAction::Run, GetOpenXRAction(state));
  EXPECT_EQ(OpenXRAction::EndSession,
            GetOpenXRAction(XR_SESSION_STATE_STOPPING));
  EXPECT_EQ(OpenXRAction::FallBack,
            GetOpenXRAction(XR_SESSION_STATE_LOSS_PENDING));
  EXPECT_EQ(OpenXRAction::FallBack, GetOpenXRAction(XR_SESSION_STATE_EXITING));
}
