#include "stdafx.h"

#include "../../src/core/common/MGDFResources.hpp"

using namespace MGDF::core;

class ResourcesTests : public ::testing::Test {
 protected:
  void SetUp() override {
    HINSTANCE inst;
    inst = (HINSTANCE)GetModuleHandleW(L"core.tests.exe");
    Resources::Instance(inst);
    Resources::Instance().SetUserBaseDir("junkship", true);
  }
};

TEST_F(ResourcesTests, PathTests) {
  std::wstring root = Resources::Instance().RootDir();

  EXPECT_EQ(root + L"game/bin/module.dll", Resources::Instance().Module());

  EXPECT_EQ(root + L"game/game.json", Resources::Instance().GameFile());
  EXPECT_EQ((root + L"game/preferences.json"),
            Resources::Instance().GameDefaultPreferencesFile());
  EXPECT_EQ(root + L"game/bin/", Resources::Instance().BinDir());
  EXPECT_EQ(root + L"game/content/", Resources::Instance().ContentDir());
  EXPECT_EQ(root + L"resources/preferences.json",
            Resources::Instance().CorePreferencesFile());

  // use the games directory override
  Resources::Instance().SetGameBaseDir(L"c:/MGDF/");
  EXPECT_EQ(L"c:/MGDF/game.json", Resources::Instance().GameFile());
  EXPECT_EQ(L"c:/MGDF/preferences.json",
            Resources::Instance().GameDefaultPreferencesFile());
  EXPECT_EQ(L"c:/MGDF/bin/", Resources::Instance().BinDir());
  EXPECT_EQ(L"c:/MGDF/content/", Resources::Instance().ContentDir());
  EXPECT_EQ(root + L"resources/preferences.json",
            Resources::Instance().CorePreferencesFile());

  EXPECT_EQ(root + L"user/junkship/", Resources::Instance().UserBaseDir());
  EXPECT_EQ(root + L"user/junkship/working/",
            Resources::Instance().WorkingDir());
  EXPECT_EQ(root + L"user/junkship/preferences.json",
            Resources::Instance().GameUserPreferencesFile());
  EXPECT_EQ(root + L"user/junkship/saves/",
            Resources::Instance().SaveBaseDir());
  EXPECT_EQ(root + L"user/junkship/saves/save1/gameState.json",
            Resources::Instance().GameStateSaveFile("save1"));
  EXPECT_EQ(root + L"user/junkship/saves/save1/",
            Resources::Instance().SaveDir("save1"));
  EXPECT_EQ(root + L"user/junkship/saves/save1/data/",
            Resources::Instance().SaveDataDir("save1"));
}