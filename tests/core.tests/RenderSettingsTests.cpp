#include "stdafx.h"

#include "../../src/core/common/MGDFWindowSizing.hpp"
#include "../../src/core/core.impl/MGDFRenderSettingsManagerImpl.hpp"

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
  const RECT client{0, 0, 1024, 768};
  for (const MGDFFullScreenDesc choice :
       {MGDFFullScreenDesc{TRUE, TRUE}, {TRUE, FALSE}, {FALSE, FALSE}}) {
    {
      ComObject<IMGDFPendingRenderSettingsChange> change;
      settings->CreatePendingSettingsChange(change.Assign());
      change->SetFullscreen(&choice);
      ASSERT_TRUE(change->SetCurrentPrimaryDisplayMode(&mode));
    }
    settings->OnResetSwapChain(swap, fullscreen, client);
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
