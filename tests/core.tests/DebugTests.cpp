#include "stdafx.h"

#include "../../src/core/core.impl/MGDFDebugImpl.hpp"

using namespace MGDF;
using namespace MGDF::core;

namespace {

std::string OverlayData(Debug &debug) {
  UINT64 length = 0;
  EXPECT_EQ(S_OK, debug.GetOverlayData(nullptr, &length));
  std::string data(static_cast<size_t>(length), '\0');
  EXPECT_EQ(S_OK, debug.GetOverlayData(data.data(), &length));
  data.resize(static_cast<size_t>(length));
  return data;
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

TEST(DebugTests, OverlayDataCarriesSectionsAsJson) {
  auto debug = MakeCom<Debug>(nullptr);
  debug->Set("Scene", "Visible", "12");
  debug->Set("Scene", "Quote \"d\"", "a\\b");
  debug->Set("Script", "Error", "none");
  debug->Clear("Script", nullptr);

  const std::string data = OverlayData(*debug.Get());
  EXPECT_NE(std::string::npos, data.find("\"version\":\""));
  EXPECT_NE(std::string::npos, data.find("\"counters\":[]"));
  EXPECT_NE(std::string::npos,
            data.find("\"sections\":{\"Scene\":{\"Quote \\\"d\\\"\":\"a\\\\b\","
                      "\"Visible\":\"12\"}}"));
  EXPECT_EQ(std::string::npos, data.find("Script"));
  EXPECT_EQ(std::string::npos, data.find("\"timings\""));
}

TEST(DebugTests, OverlayDataReportsRequiredLength) {
  auto debug = MakeCom<Debug>(nullptr);
  UINT64 length = 4;
  char tiny[4];
  EXPECT_EQ(E_NOT_SUFFICIENT_BUFFER, debug->GetOverlayData(tiny, &length));
  EXPECT_GT(length, 4u);
}
