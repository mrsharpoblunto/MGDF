#include "stdafx.h"

#include <thread>

#include "../../src/core/input/xinput/XInputManagerComponentImpl.hpp"

using namespace MGDF;
using namespace MGDF::core::input::xinput;

namespace {
void Key(XInputManagerComponent &input, USHORT flags) {
  RAWINPUT event{};
  event.header.dwType = RIM_TYPEKEYBOARD;
  event.data.keyboard.VKey = VK_SPACE;
  event.data.keyboard.Flags = flags;
  bool pending = true;
  input.HandleInput([&]() -> RAWINPUT * {
    return std::exchange(pending, false) ? &event : nullptr;
  });
}
}  // namespace

TEST(InputTests, KeyReleaseSurvivesAnOverflowLengthEventSequence) {
  const auto input = MakeCom<XInputManagerComponent>();
  Key(*input.Get(), RI_KEY_MAKE);
  input->ProcessSim();
  ASSERT_TRUE(input->IsKeyDown(VK_SPACE));
  for (UINT32 i = 0; i < 255; ++i)
    Key(*input.Get(), i % 2 ? RI_KEY_MAKE : RI_KEY_BREAK);
  input->ProcessSim();
  EXPECT_TRUE(input->IsKeyUp(VK_SPACE));
  EXPECT_TRUE(input->IsKeyPress(VK_SPACE));
  input->ProcessSim();
  EXPECT_TRUE(input->IsKeyUp(VK_SPACE));
  EXPECT_FALSE(input->IsKeyPress(VK_SPACE));
}

TEST(InputTests, ClearIsAppliedAtTheNextSimulationBoundary) {
  const auto input = MakeCom<XInputManagerComponent>();
  Key(*input.Get(), RI_KEY_MAKE | RI_KEY_E0);
  input->ProcessSim();
  ASSERT_TRUE(input->IsKeyDown(VK_SPACE));
  Key(*input.Get(), RI_KEY_BREAK | RI_KEY_E0);
  std::thread window([&]() { input->ClearInput(); });
  window.join();
  EXPECT_TRUE(input->IsKeyDown(VK_SPACE));
  input->ProcessSim();
  EXPECT_TRUE(input->IsKeyUp(VK_SPACE));
  EXPECT_FALSE(input->IsKeyPress(VK_SPACE));
  Key(*input.Get(), RI_KEY_MAKE);
  input->ProcessSim();
  EXPECT_TRUE(input->IsKeyDown(VK_SPACE));
}
