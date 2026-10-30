#include "stdafx.h"

#include "../../src/core/common/MGDFWindowSizing.hpp"
#include "../../src/core/core.impl/MGDFGameImpl.hpp"
#include "../../src/core/core.impl/MGDFRenderSettingsManagerImpl.hpp"
#include "../../src/core/storage/jsoncpp/JsonStorageFactoryComponent.hpp"

using namespace MGDF;
using namespace MGDF::core;

TEST(RenderSettings, CommitRetainsDisplayModeAndSchedulesReset) {
  auto settings = MakeCom<RenderSettingsManager>();
  const MGDFDisplayMode mode{1280, 720, 60, 1, FALSE, FALSE};
  settings->SetOutputProperties({}, 96, {mode});
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    ASSERT_TRUE(change->SetCurrentPrimaryDisplayMode(&mode));
  }
  MGDFDisplayMode current{};
  settings->GetCurrentPrimaryDisplayMode(&current);
  EXPECT_EQ(current.Width, 1280u);
  EXPECT_EQ(current.Height, 720u);
  EXPECT_TRUE(settings->IsBackBufferChangePending());
}

TEST(RenderSettings, CancelDiscardsDisplaySizeAndAspectChanges) {
  auto settings = MakeCom<RenderSettingsManager>();
  const MGDFDisplayMode mode{1280, 720, 60, 1, FALSE, FALSE};
  settings->SetOutputProperties({}, 96, {mode});
  MGDFDisplayMode before{};
  settings->GetCurrentPrimaryDisplayMode(&before);
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    ASSERT_TRUE(change->SetCurrentPrimaryDisplayMode(&mode));
    change->SetWindowSize(1280, 720);
    change->SetWindowAspectLock(TRUE);
    change->Cancel();
  }
  MGDFDisplayMode current{};
  settings->GetCurrentPrimaryDisplayMode(&current);
  EXPECT_EQ(current.Width, before.Width);
  EXPECT_EQ(current.Height, before.Height);
  EXPECT_FALSE(settings->TakeWindowSize());
  EXPECT_FALSE(settings->GetWindowAspectLock());
}

TEST(RenderSettings, WindowSizeWaitsForTheActualClientResize) {
  auto settings = MakeCom<RenderSettingsManager>();
  settings->OnResize(1920, 1080);
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    change->SetWindowSize(1280, 720);
  }
  const auto size = settings->TakeWindowSize();
  ASSERT_TRUE(size);
  EXPECT_EQ(size->first, 1280u);
  EXPECT_EQ(size->second, 720u);
  EXPECT_TRUE(settings->IsBackBufferChangePending());
  EXPECT_EQ(settings->GetScreenX(), 1920u);
  EXPECT_FALSE(settings->TakeWindowSize());
  settings->OnResize(size->first, size->second);
  EXPECT_EQ(settings->GetScreenX(), 1280u);
  EXPECT_EQ(settings->GetScreenY(), 720u);
}

TEST(RenderSettings, AspectLockCommitsThroughTheSettingsInterface) {
  auto settings = MakeCom<RenderSettingsManager>();
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    change->SetWindowAspectLock(TRUE);
  }
  ComObject<IMGDFRenderSettingsManager> current(settings.Get(), true);
  EXPECT_TRUE(current->GetWindowAspectLock());
}

TEST(RenderSettings, ResetUsesExclusiveModeOrTheActualWindowSize) {
  auto settings = MakeCom<RenderSettingsManager>();
  settings->InitD3D12();
  const MGDFDisplayMode mode{1280, 720, 60, 1, FALSE, FALSE};
  settings->SetOutputProperties({.Width = 1920, .Height = 1080}, 96, {mode});
  DXGI_SWAP_CHAIN_DESC1 swap{};
  DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen{};
  DXGI_COLOR_SPACE_TYPE colorSpace{};
  const RECT client{0, 0, 1024, 768};
  for (const MGDFFullScreenDesc choice :
       {MGDFFullScreenDesc{TRUE, TRUE}, {TRUE, FALSE}, {FALSE, FALSE}}) {
    {
      ComObject<IMGDFPendingRenderSettingsChange> change;
      settings->CreatePendingSettingsChange(change.Assign());
      change->SetFullscreen(&choice);
      ASSERT_TRUE(change->SetCurrentPrimaryDisplayMode(&mode));
    }
    settings->OnResetSwapChain(swap, fullscreen, colorSpace, client);
    EXPECT_EQ(fullscreen.Windowed,
              !(choice.FullScreen && choice.ExclusiveMode));
    EXPECT_EQ(swap.Width, choice.FullScreen
                              ? (choice.ExclusiveMode ? 1280u : 1920u)
                              : 1024u);
    EXPECT_EQ(swap.Height,
              choice.FullScreen ? (choice.ExclusiveMode ? 720u : 1080u) : 768u);
    EXPECT_FALSE(settings->IsBackBufferChangePending());
  }
}

TEST(RenderSettings, HDRFormatSelectsTheHDRBackBufferAndColorSpace) {
  struct Case {
    MGDFHDRFormat Format;
    BOOL OutputHDR;
    DXGI_FORMAT ExpectedFormat;
    DXGI_COLOR_SPACE_TYPE ExpectedColorSpace;
  };
  const Case cases[] = {
      {MGDF_HDR_FORMAT_SCRGB, TRUE, DXGI_FORMAT_R16G16B16A16_FLOAT,
       DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709},
      {MGDF_HDR_FORMAT_HDR10, TRUE, DXGI_FORMAT_R10G10B10A2_UNORM,
       DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020},
      {MGDF_HDR_FORMAT_HDR10, FALSE, DXGI_FORMAT_R8G8B8A8_UNORM,
       DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709},
  };
  for (const auto &c : cases) {
    SCOPED_TRACE(c.Format);
    auto settings = MakeCom<RenderSettingsManager>();
    settings->SetHDRFormat(c.Format);
    settings->InitD3D12();
    const MGDFDisplayMode mode{1280, 720, 60, 1, c.OutputHDR, FALSE};
    settings->SetOutputProperties(
        {.SupportsHDR = c.OutputHDR, .Width = 1280, .Height = 720}, 96, {mode});
    {
      ComObject<IMGDFPendingRenderSettingsChange> change;
      settings->CreatePendingSettingsChange(change.Assign());
      change->SetHDREnabled(TRUE);
    }
    DXGI_SWAP_CHAIN_DESC1 swap{};
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen{};
    DXGI_COLOR_SPACE_TYPE colorSpace{};
    settings->OnResetSwapChain(swap, fullscreen, colorSpace, {0, 0, 640, 480});
    EXPECT_EQ(c.ExpectedFormat, swap.Format);
    EXPECT_EQ(c.ExpectedColorSpace, colorSpace);
  }
}

TEST(RenderSettings, FrameFeatureChangesNeedDeviceSupport) {
  auto settings = MakeCom<RenderSettingsManager>();
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    EXPECT_FALSE(change->SetLowLatencyMode(MGDF_LOW_LATENCY_ON));
    EXPECT_FALSE(change->SetFrameGeneration(1));
    EXPECT_TRUE(change->SetLowLatencyMode(MGDF_LOW_LATENCY_OFF));
    EXPECT_TRUE(change->SetFrameGeneration(0));
  }
  EXPECT_FALSE(settings->IsBackBufferChangePending());

  settings->SetGraphicsFeatureSupport(
      {MGDF_GRAPHICS_FEATURE_LOW_LATENCY |
           MGDF_GRAPHICS_FEATURE_FRAME_GENERATION,
       2, TRUE});
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    EXPECT_FALSE(change->SetFrameGeneration(3));
    EXPECT_FALSE(change->SetLowLatencyMode(static_cast<MGDFLowLatencyMode>(3)));
    EXPECT_TRUE(change->SetLowLatencyMode(MGDF_LOW_LATENCY_BOOST));
  }
  EXPECT_EQ(MGDF_LOW_LATENCY_BOOST, settings->GetLowLatencyMode());
  EXPECT_FALSE(settings->IsBackBufferChangePending());
  {
    ComObject<IMGDFPendingRenderSettingsChange> change;
    settings->CreatePendingSettingsChange(change.Assign());
    EXPECT_TRUE(change->SetFrameGeneration(2));
  }
  EXPECT_EQ(2u, settings->GetFrameGeneration());
  EXPECT_TRUE(settings->IsBackBufferChangePending());

  settings->SetGraphicsFeatureSupport(
      {MGDF_GRAPHICS_FEATURE_FRAME_GENERATION, 1, FALSE});
  EXPECT_EQ(1u, settings->GetFrameGeneration());
  EXPECT_EQ(MGDF_LOW_LATENCY_OFF, settings->GetLowLatencyMode());
}

TEST(RenderSettings, FrameFeaturePreferencesPersistWithinSupport) {
  std::shared_ptr<storage::IStorageFactoryComponent> storage;
  storage::json::CreateStorageFactoryComponent(storage);
  auto game = MakeCom<Game>("settings-tests", "Settings tests", "",
                            MGDFVersion{}, storage);
  const MGDFDisplayMode mode{1280, 720, 60, 1, FALSE, FALSE};
  const MGDFGraphicsFeatureSupport full{
      MGDF_GRAPHICS_FEATURE_LOW_LATENCY |
          MGDF_GRAPHICS_FEATURE_FRAME_GENERATION,
      3, TRUE};
  const auto create = [&](const MGDFGraphicsFeatureSupport &support) {
    auto settings = MakeCom<RenderSettingsManager>();
    settings->InitD3D12();
    settings->SetOutputProperties({}, 96, {mode});
    settings->SetGraphicsFeatureSupport(support);
    return settings;
  };
  {
    auto settings = create(full);
    {
      ComObject<IMGDFPendingRenderSettingsChange> change;
      settings->CreatePendingSettingsChange(change.Assign());
      ASSERT_TRUE(change->SetCurrentPrimaryDisplayMode(&mode));
      ASSERT_TRUE(change->SetLowLatencyMode(MGDF_LOW_LATENCY_BOOST));
      ASSERT_TRUE(change->SetFrameGeneration(3));
    }
    ComObject<IMGDFPreferenceSet> preferences;
    settings->GetPreferences(preferences.Assign());
    game->SetPreferences(preferences);
  }
  auto loaded = create(full);
  loaded->LoadPreferences(game.As<IMGDFGame>());
  EXPECT_EQ(MGDF_LOW_LATENCY_BOOST, loaded->GetLowLatencyMode());
  EXPECT_EQ(3u, loaded->GetFrameGeneration());

  auto limited = create({MGDF_GRAPHICS_FEATURE_FRAME_GENERATION, 1, FALSE});
  limited->LoadPreferences(game.As<IMGDFGame>());
  EXPECT_EQ(MGDF_LOW_LATENCY_OFF, limited->GetLowLatencyMode());
  EXPECT_EQ(1u, limited->GetFrameGeneration());
}

TEST(WindowSizing, EveryEdgeKeepsClientAspectAndTheOppositeAnchor) {
  const RECT proposed{100, 100, 916, 639};
  for (UINT edge = WMSZ_LEFT; edge <= WMSZ_BOTTOMRIGHT; ++edge) {
    SCOPED_TRACE(edge);
    const auto rect = KeepClientAspect(proposed, edge, 16, 39, 16.0 / 9.0);
    const LONG width = rect.right - rect.left - 16;
    const LONG height = rect.bottom - rect.top - 39;
    EXPECT_NEAR(static_cast<double>(width) / height, 16.0 / 9.0, 0.002);
    if (edge == WMSZ_LEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_BOTTOMLEFT)
      EXPECT_EQ(rect.right, proposed.right);
    else
      EXPECT_EQ(rect.left, proposed.left);
    if (edge == WMSZ_TOP || edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT)
      EXPECT_EQ(rect.bottom, proposed.bottom);
    else
      EXPECT_EQ(rect.top, proposed.top);
  }
}
