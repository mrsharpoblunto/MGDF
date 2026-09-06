#include "stdafx.h"

#include "../../src/core/common/MGDFVersionInfo.hpp"
#include "../../src/core/core.impl/MGDFDebugImpl.hpp"

using namespace MGDF;
using namespace MGDF::core;

namespace {

ComObject<IMGDFDebugOverlaySnapshot> Snapshot(Debug &debug) {
  ComObject<IMGDFDebugOverlaySnapshot> snapshot;
  EXPECT_EQ(S_OK, debug.GetOverlaySnapshot(snapshot.Assign()));
  return snapshot;
}

}  // namespace

TEST(DebugTests, HostRenderingDefaultsOnAndToggles) {
  auto debug = MakeCom<Debug>(nullptr);
  EXPECT_TRUE(debug->IsHostRenderingEnabled());
  debug->SetHostRenderingEnabled(FALSE);
  EXPECT_FALSE(debug->IsHostRenderingEnabled());
  EXPECT_FALSE(debug->IsShown());
  debug->ToggleShown();
  EXPECT_TRUE(debug->IsShown());
}

TEST(DebugTests, OverlaySnapshotCarriesSections) {
  auto debug = MakeCom<Debug>(nullptr);
  debug->Set("Scene", "Visible", "12");
  debug->Set("Scene", "Bodies", "3");
  debug->Set("Script", "Error", "none");
  debug->Clear("Script", nullptr);

  const auto snapshot = Snapshot(*debug.Get());
  const auto data = snapshot->GetData();
  EXPECT_STREQ(MGDFVersionInfo::MGDF_VERSION(), data->Version);
  EXPECT_EQ(MGDFVersionInfo::MGDF_INTERFACE_VERSION, data->InterfaceVersion);
  EXPECT_FALSE(data->HasTimings);
  EXPECT_EQ(0u, data->CounterCount);
  ASSERT_EQ(2u, data->EntryCount);
  EXPECT_STREQ("Scene", data->Entries[0].Section);
  EXPECT_STREQ("Bodies", data->Entries[0].Key);
  EXPECT_STREQ("3", data->Entries[0].Value);
  EXPECT_STREQ("Scene", data->Entries[1].Section);
  EXPECT_STREQ("Visible", data->Entries[1].Key);
  EXPECT_STREQ("12", data->Entries[1].Value);
}

TEST(DebugTests, OverlaySnapshotOutlivesLaterChanges) {
  auto debug = MakeCom<Debug>(nullptr);
  debug->Set("Scene", "Visible", "12");
  const auto snapshot = Snapshot(*debug.Get());
  debug->Clear("Scene", nullptr);
  debug->Set("Other", "Key", "value");

  const auto data = snapshot->GetData();
  ASSERT_EQ(1u, data->EntryCount);
  EXPECT_STREQ("Scene", data->Entries[0].Section);
  EXPECT_STREQ("12", data->Entries[0].Value);
}

TEST(DebugTests, OverlaySnapshotCopiesFrameTimings) {
  HostMetrics metrics(2);
  metrics.SetExpectedSimTime(0.02);
  metrics.AppendRenderTimes(0.010, 0.004);
  metrics.AppendRenderTimes(0.020, 0.006);
  metrics.AppendSimTime(0.015);

  auto debug = MakeCom<Debug>(nullptr);
  debug->SetMetrics(&metrics);
  const auto snapshot = Snapshot(*debug.Get());
  debug->SetMetrics(nullptr);

  const auto data = snapshot->GetData();
  EXPECT_TRUE(data->HasTimings);
  EXPECT_DOUBLE_EQ(0.02, data->ExpectedSimTime);
  EXPECT_DOUBLE_EQ(0.015, data->RenderTime.Average);
  ASSERT_EQ(2u, data->RenderTime.SampleCount);
  EXPECT_DOUBLE_EQ(0.020, data->RenderTime.Samples[0]);
  EXPECT_DOUBLE_EQ(0.010, data->RenderTime.Samples[1]);
  EXPECT_DOUBLE_EQ(0.005, data->ActiveRenderTime.Average);
  ASSERT_EQ(1u, data->SimTime.SampleCount);
  EXPECT_DOUBLE_EQ(0.015, data->SimTime.Samples[0]);
  EXPECT_EQ(0u, data->SimInputTime.SampleCount);
}
